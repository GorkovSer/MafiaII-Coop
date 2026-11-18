/* Opt-in diagnostic: ask the game's own Continue backend to load an existing
   main-story checkpoint. Never initializes freeride or disables story saves.
   Only called below the observer's whole-EXE hash gate, on its game thread. */
#define WIN32_LEAN_AND_MEAN
#include "story_test.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

int CoopProfileWasRedirected(void);
static wchar_t evidence[MAX_PATH];
static int enabled, requested, waiting_for_saves, waiting_for_menu;
static ULONGLONG first_menu_tick, menu_ready_since;

static int readable(const void *pointer,size_t bytes) {
    MEMORY_BASIC_INFORMATION region;
    if(!pointer || VirtualQuery(pointer,&region,sizeof(region))!=sizeof(region) ||
       region.State!=MEM_COMMIT || (region.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return 0;
    uintptr_t at=(uintptr_t)pointer,end=(uintptr_t)region.BaseAddress+region.RegionSize;
    return end>=at && bytes<=end-at;
}
static void record(const char *event,unsigned int value) {
    char line[160];
    int size=snprintf(line,sizeof(line),"story_test=%s value=%u thread=%lu\r\n",event,value,(unsigned long)GetCurrentThreadId());
    if(size<=0 || (size_t)size>=sizeof(line)) return;
    HANDLE file=CreateFileW(evidence,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
    if(file==INVALID_HANDLE_VALUE)return;
    DWORD written;WriteFile(file,line,(DWORD)size,&written,NULL);CloseHandle(file);
}
int CoopStoryTestStart(const wchar_t *log_path) {
    if(!log_path || wcslen(log_path)>=MAX_PATH)return 1;
    wcscpy(evidence,log_path);
    wchar_t ini[MAX_PATH];wcscpy(ini,log_path);
    wchar_t *tail=wcsrchr(ini,L'\\');if(!tail)return 1;
    *tail=0;
    if(wcslen(ini)+wcslen(L"\\story-test.ini")>=MAX_PATH)return 1;
    wcscat(ini,L"\\story-test.ini");
    enabled=GetPrivateProfileIntW(L"test",L"continue_main_story",0,ini)==1;
    if(!enabled)return 0;
    static const unsigned char prefix[]={0x83,0xec,0x08,0x53,0x8d,0x44,0x24,0x04,0x50,0x8b,0xd9};
    const void *target=(void *)(uintptr_t)0x473c80;
    if(!readable(target,sizeof(prefix)) || memcmp(target,prefix,sizeof(prefix)))return 2;
    /* Installed GOG binary: cdecl, no arguments, AL result, plain ret on
       all paths. Queries whether the current GUI daemon is MainMenu. */
    static const unsigned char menu_prefix[]={0x83,0xec,0x08,0x53,0xe8,0x87,0x58,0x3d,0x00};
    target=(void *)(uintptr_t)0x48c9c0;
    if(!readable(target,sizeof(menu_prefix)) || memcmp(target,menu_prefix,sizeof(menu_prefix)))return 3;
    record("continue_enabled",1);
    return 0;
}
void CoopStoryTestEvent(unsigned int id) {
    if(!enabled || requested || id!=22)return;
    ULONGLONG now=GetTickCount64();
    if(!first_menu_tick){first_menu_tick=now;return;}
    if(now-first_menu_tick<10000)return;
    if(!CoopProfileWasRedirected()){requested=1;record("refused_unisolated_profile",0);return;}
    void *base=*(void **)(uintptr_t)0x1ab2838;
    void *saves=*(void **)(uintptr_t)0x1ac0c50;
    if(!readable(base,0xd8) || !readable(saves,0x14))return;
    /* Continue cannot find a checkpoint before the profile's save catalogue
       has been enumerated. An empty vector is a wait, not a load attempt. */
    uintptr_t begin=*(uintptr_t *)((char *)saves+0x0c);
    uintptr_t end=*(uintptr_t *)((char *)saves+0x10);
    if(!begin || end<=begin || (end-begin)%4 || (end-begin)/4>256 ||
       !readable((void *)begin,end-begin)) {
        if(!waiting_for_saves){record("waiting_for_save_catalogue",0);waiting_for_saves=1;}
        return;
    }
    /* Save enumeration precedes DLC enumeration. Wait for the actual front
       end as well: the earlier LoadingDLC attempt reported missing DLC. */
    typedef unsigned char (__attribute__((cdecl)) *menu_type)(void);
    menu_type in_menu=(menu_type)(uintptr_t)0x48c9c0;
    if(!in_menu()) {
        menu_ready_since=0;
        if(!waiting_for_menu){record("waiting_for_main_menu",0);waiting_for_menu=1;}
        return;
    }
    if(!menu_ready_since){menu_ready_since=now;return;}
    if(now-menu_ready_since<1000)return;
    requested=1;
    /* CGuiGame::StartGame(TYPE_CONTINUE,...,category) dispatches through
       0x450c10 -> 0x473c80 with ECX=global 0x1ab2838 and one stack argument.
       Save lookup 0x11c3690 treats category 1 as main-story IDs 1/100..1999.
       Both return paths use ret 4; AL is the accepted/not-found result. */
    typedef unsigned char (__attribute__((thiscall)) *continue_type)(void *,unsigned int);
    continue_type load=(continue_type)(uintptr_t)0x473c80;
    record("continue_requested",1);
    unsigned char result=load(base,1);
    record("continue_returned",result);
}
