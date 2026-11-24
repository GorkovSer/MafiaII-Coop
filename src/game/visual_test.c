/* Opt-in FrameWrapper lifecycle/pose diagnostic. Never call Human/AI APIs on it.
   The observer establishes the full pinned EXE hash before this module starts. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "visual_test.h"
#include "entity_registry.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int CoopProfileWasRedirected(void);
static wchar_t log_path[MAX_PATH];
static int enabled, initialized, loaded, paused, attempted, busy, invalidate;
static uint32_t entity, guid, model, pending;
static DWORD owner_thread;
static ULONGLONG ready_at, born_at, next_pose, next_sample;
static float origin[3];
static int last_phase=-1;
typedef void *(__attribute__((stdcall)) *ModelFactory)(void *);
typedef void *(__attribute__((cdecl)) *EntityFactory)(unsigned int);
typedef int (__attribute__((thiscall)) *Clone)(void *, void *);
typedef unsigned int (__attribute__((thiscall)) *ModelRelease)(void *);
typedef void *(__attribute__((thiscall)) *SetModel)(void *, void *);
typedef void (__attribute__((thiscall)) *EntityCall)(void *);
typedef void (__attribute__((thiscall)) *SetVector)(void *, const float *);

static int read_at(void *context, uint32_t at, void *out, size_t size) {
    (void)context;
    MEMORY_BASIC_INFORMATION r;
    if (!at || VirtualQuery((void *)(uintptr_t)at,&r,sizeof(r))!=sizeof(r) ||
        r.State!=MEM_COMMIT || (r.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return 0;
    uintptr_t end=(uintptr_t)r.BaseAddress+r.RegionSize;
    if (end<at || size>end-at) return 0;
    memcpy(out,(void *)(uintptr_t)at,size); return 1;
}
static int word(uint32_t base, uint32_t offset, uint32_t *out) {
    return base && offset<=UINT32_MAX-base && read_at(NULL,base+offset,out,4);
}
static void append(const char *line, int n) {
    if(n<=0 || n>=320) return;
    HANDLE f=CreateFileW(log_path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,OPEN_EXISTING,0,NULL);
    if(f==INVALID_HANDLE_VALUE) return;
    DWORD written; WriteFile(f,line,(DWORD)n,&written,NULL); CloseHandle(f);
}
static void record(const char *stage, uint32_t value) {
    char line[160]; int n=snprintf(line,sizeof(line),
        "visual_test=%s value=%lu ms=%llu thread=%lu\r\n",stage,(unsigned long)value,
        (unsigned long long)GetTickCount64(),(unsigned long)GetCurrentThreadId());
    append(line,n);
}
static CoopEntityStatus lookup(uint32_t id, uint32_t *out) {
    uint32_t manager,table,method;
    *out=0;
    if(!word(0x1abe8d8,0,&manager) || !word(manager,0,&table) ||
        !word(table,0x1c,&method) || method!=0x5a0c70) return COOP_ENTITY_INVALID;
    return CoopEntityLookup(read_at,NULL,manager,id,out);
}
static int valid(void) {
    uint32_t current,table,id,current_model;
    return entity && GetCurrentThreadId()==owner_thread && CoopProfileWasRedirected() &&
        (guid&255)==0x37 && lookup(guid,&current)==COOP_ENTITY_PRESENT && current==entity &&
        word(entity,0,&table) && table==0x196d6e0 && word(entity,0x1c,&id) && id==guid &&
        (!model || (word(entity,0x60,&current_model) && current_model==model));
}
static void poll_removed(void) {
    uint32_t current;
    if(pending && lookup(pending,&current)==COOP_ENTITY_ABSENT) {
        record("removal_registry_confirmed",pending); pending=0;
    }
}
static void retire(void) {
    if(!entity) return;
    if(GetCurrentThreadId()!=owner_thread) return; /* Keep ownership for the game thread. */
    if(!valid()) {record("lost_identity_no_release",guid); entity=model=0; return;}
    uint32_t retiring=entity;
    pending=guid; entity=model=0; /* No reads or second release after this call. */
    record("release_requested",pending);
    ((EntityCall)(uintptr_t)0x119ddd0)((void *)(uintptr_t)retiring);
    record("release_returned",pending); poll_removed();
}
static int vector_ok(const float p[3]) {
    for(unsigned int i=0;i<3;++i) if(!isfinite(p[i]) || fabsf(p[i])>=1000000) return 0;
    return 1;
}
static int local_source(uint32_t *source, float p[3]) {
    uint32_t game,player,table,physics,physics_table,getter;
    return word(0x1abfe14,0,&game) && word(game,0x118,&player) &&
        word(player,0,&table) && table==0x18f14b8 &&
        word(player,0x60,source) && word(*source,0,&table) && table==0x191c850 &&
        word(player,0x190,&physics) && word(physics,0,&physics_table) &&
        word(physics_table,0x54,&getter) && getter==0x18ae690 &&
        physics<=UINT32_MAX-0x1e4 && read_at(NULL,physics+0x1d8,p,12) && vector_ok(p);
}
static void create(void) {
    uint32_t source,parent,link;
    if(!CoopProfileWasRedirected() || !local_source(&source,origin)) return;
    attempted=1; owner_thread=GetCurrentThreadId();
    origin[0]-=2; origin[1]+=2;
    void *copy=((ModelFactory)(uintptr_t)0x14ec940)(NULL);
    if(!copy) {record("model_allocation_failed",0); return;}
    int status=((Clone)(uintptr_t)0x14ec8f0)(copy,(void *)(uintptr_t)source);
    if(status<0 || invalidate || !loaded || !initialized || paused ||
        !word((uint32_t)(uintptr_t)copy,0x1c,&parent) || parent ||
        !word((uint32_t)(uintptr_t)copy,0x20,&link) || link) {
        ((ModelRelease)(uintptr_t)0x14e5bc0)(copy);
        record("clone_not_detached_or_context_changed",0); return;
    }
    void *created=((EntityFactory)(uintptr_t)0x1167a60)(0x37);
    uint32_t table,empty_model;
    if(!created || !word((uint32_t)(uintptr_t)created,0,&table) || table!=0x196d6e0 ||
        !word((uint32_t)(uintptr_t)created,0x60,&empty_model) || empty_model ||
        !word((uint32_t)(uintptr_t)created,0x1c,&guid)) {
        ((ModelRelease)(uintptr_t)0x14e5bc0)(copy);
        record("factory_contract_incomplete",0); return;
    }
    entity=(uint32_t)(uintptr_t)created;
    if(!valid()) {
        entity=0; ((ModelRelease)(uintptr_t)0x14e5bc0)(copy);
        record("creation_registry_refused",0); return;
    }
    record("creation_registry_confirmed",guid);
    ((SetModel)(uintptr_t)0x1164d00)(created,copy);
    *(uint32_t *)((char *)created+0x20)|=0x800; /* Transfer the single owned model reference. */
    model=(uint32_t)(uintptr_t)copy; copy=NULL;
    if(invalidate || !loaded || !initialized || paused) {retire(); return;}
    ((SetVector)(uintptr_t)0x1164d40)(created,origin);
    ((EntityCall)(uintptr_t)0x11a31b0)(created); /* Common game-entity setup, not Human Setup. */
    if(!valid() || invalidate || !loaded || !initialized || paused) {retire(); return;}
    ((EntityCall)(uintptr_t)0x1192170)(created); /* Dispatches FrameWrapper activation. */
    uint32_t flags;
    if(!valid() || !word(entity,0x20,&flags) || !(flags&1)) {
        record("activation_incomplete",0); retire(); return;
    }
    record("active",guid); born_at=GetTickCount64(); next_pose=next_sample=0;
}
static void tick(void) {
    ULONGLONG now=GetTickCount64();
    if(!valid()) {retire(); return;}
    if(now-born_at>=15000) {retire(); return;}
    if(now<next_pose) return;
    next_pose=now+50;
    float t=(float)(now-born_at)/1000.0f;
    float p[3]={origin[0],origin[1],origin[2]}, direction[3]={1,0,0};
    int phase=t<4 ? 0 : t<7 ? 1 : t<11 ? 2 : 3;
    if(phase==0) p[0]+=fmaxf(0,t-1)*2.0f/3.0f;
    else {p[0]+=2; direction[0]=0; direction[1]=1;}
    if(phase>=2) p[1]+=fminf(4,t-7)*0.5f;
    if(phase!=last_phase) {last_phase=phase; record("phase",(uint32_t)phase);}
    ((SetVector)(uintptr_t)0x1164d40)((void *)(uintptr_t)entity,p);
    if(invalidate || !valid()) {retire(); return;}
    ((SetVector)(uintptr_t)0x1175120)((void *)(uintptr_t)entity,direction);
    if(now>=next_sample && valid()) {
        next_sample=now+500;
        float xyz[3],x,y;
        if(word(model,0x44,(uint32_t *)&xyz[0]) && word(model,0x54,(uint32_t *)&xyz[1]) &&
            word(model,0x64,(uint32_t *)&xyz[2]) && word(model,0x3c,(uint32_t *)&x) &&
            word(model,0x4c,(uint32_t *)&y) && vector_ok(xyz) && isfinite(x) && isfinite(y)) {
            char line[320]; int n=snprintf(line,sizeof(line),
                "visual_sample ms=%llu phase=%d xyz=%.5f,%.5f,%.5f heading=%.6f\r\n",
                (unsigned long long)now,phase,(double)xyz[0],(double)xyz[1],(double)xyz[2],
                (double)atan2f(y,x)); append(line,n);
        }
    }
}
int CoopVisualTestStart(const wchar_t *path) {
    if(!path || wcslen(path)>=MAX_PATH) return 1;
    wcscpy(log_path,path);
    wchar_t ini[MAX_PATH]; wcscpy(ini,path);
    wchar_t *tail=wcsrchr(ini,L'\\'); if(!tail) return 1; *tail=0;
    size_t length=wcslen(ini);
    if(length+32>=MAX_PATH) return 1;
    wcscat(ini,L"\\visual-test.ini");
    enabled=GetPrivateProfileIntW(L"test",L"visual_once",0,ini)==1;
    if(!enabled) return 0;
    const wchar_t *conflicts[]={L"\\coop-session.ini",L"\\human-test.ini",
        L"\\model-test.ini",L"\\story-test.ini"};
    for(unsigned int i=0;i<sizeof(conflicts)/sizeof(conflicts[0]);++i) {
        ini[length]=0; wcscat(ini,conflicts[i]);
        DWORD attributes=GetFileAttributesW(ini),error=GetLastError();
        if(attributes!=INVALID_FILE_ATTRIBUTES ||
            (error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND)) {enabled=0; return 2;}
    }
    record("enabled_offline",1); return 0;
}
void CoopVisualTestEvent(unsigned int id) {
    if(!enabled) return;
    int close=id==10 || id==11 || id==12 || id==14 || id==32 || id==39;
    if(id==10 || id==11 || id==12 || id==14 || id==39) initialized=loaded=0;
    if(id==13) initialized=1;
    if(id==32) {loaded=0; ready_at=0;}
    if(id==33) loaded=1;
    if(id==34 || id==4) paused=1;
    if(id==35) paused=0;
    if(close) invalidate=1;
    if(busy) return;
    busy=1; poll_removed();
    if(invalidate) retire();
    else if(id==3 && initialized && loaded && !paused) {
        if(entity) tick();
        else if(!attempted && !pending) {
            if(!ready_at) ready_at=GetTickCount64()+3000;
            if(GetTickCount64()>=ready_at) create();
        }
    }
    if(invalidate) {retire(); if(!entity) invalidate=0;}
    busy=0;
}
