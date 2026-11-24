/* Isolated memory fixture for the production read-only adapter. This is not
   Mafia II, a simulated game client, or evidence of gameplay. No game code is
   loaded: numeric vtable targets are checked as data and never executed. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../src/game/telemetry.h"
#include "../src/game/player_control.h"

static int control_reads, control_fail;
static uint32_t control_change, control_mode, control_table = 0x18f14b8;
static uint32_t control_query = 0x41fcd0, control_setter = 0x438230;
static int control_read(void *context, uint32_t address, void *out, size_t size) {
    (void)context;
    ++control_reads;
    if (control_fail == control_reads || size != 4) return 0;
    uint32_t value;
    switch (address) {
    case 0x1000: value=control_table; break;
    case 0x101c: value=0x123410; break;
    case 0x1310: value=control_mode; break;
    case 0x18f1604: value=control_query; break;
    case 0x18f1620: value=control_setter; break;
    default: return 0;
    }
    if (address==control_change && control_reads>5) ++value;
    memcpy(out,&value,4); return 1;
}

static uint32_t game[0x120/4], player[0x198/4], player_table[0x90/4];
static uint32_t physics[0x1e8/4], physics_table[0x58/4];
static uint32_t model[0xa0/4];
static int facing_valid;
static float facing;
static int active, publications, stopped;
static uint32_t published_epoch;
static uintptr_t game_pointer;
static ULONGLONG now=1000;

uintptr_t CoopTelemetryTestGameSlot(void) {return (uintptr_t)&game_pointer;}
ULONGLONG CoopTelemetryTestNow(void) {return now;}

int CoopStoryTestStart(const wchar_t *path) {(void)path; return 0;}
void CoopStoryTestEvent(unsigned int event) {(void)event;}
int CoopNetworkStart(const wchar_t *path) {(void)path; return 0;}
void CoopNetworkStop(void) {stopped=1;}
void CoopNetworkPublish(uint32_t epoch,const float *position) {
    published_epoch=epoch; active=position!=NULL; ++publications;
    if(position && (position[0]!=1 || position[1]!=2 || position[2]!=3)) active=-1;
}
void CoopNetworkPublishPose(uint32_t epoch,const float *position,const float *heading) {
    CoopNetworkPublish(epoch,position);
    facing_valid=heading!=NULL; facing=heading ? *heading : 0;
}
static int tick_expects(int expected) {
    now+=50; CoopTelemetryEvent(3); return active==expected;
}
#define CHECK(test) do {if(!(test)){fprintf(stderr,"FAIL line %d\n",__LINE__);return 1;}} while(0)
int main(void) {
    CoopPlayerControl control;
    CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123410,&control)==COOP_PLAYER_CONTROL_VALID);
    CHECK(control.mode==0 && !control.native_lock_query);
    control_mode=10; control_reads=0;
    CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123410,&control)==COOP_PLAYER_CONTROL_VALID);
    CHECK(control.mode==10 && control.native_lock_query);
    uint32_t changes[]={0x1000,0x101c,0x1310,0x18f1604,0x18f1620};
    for (unsigned int i=0;i<sizeof(changes)/sizeof(changes[0]);++i) {
        control_change=changes[i]; control_reads=0;
        CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123410,&control)!=COOP_PLAYER_CONTROL_VALID);
        CHECK(!control.mode && !control.native_lock_query);
    }
    control_change=0;
    for (int i=1;i<=10;++i) {
        control_reads=0; control_fail=i; control.mode=99; control.native_lock_query=1;
        CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123410,&control)==COOP_PLAYER_CONTROL_UNREADABLE);
        CHECK(!control.mode && !control.native_lock_query);
    }
    control_fail=0; control_table=0x1967058;
    CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123410,&control)!=COOP_PLAYER_CONTROL_VALID);
    control_table=0x18f14b8;
    control_query=0x91aaa0;
    CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123410,&control)==COOP_PLAYER_CONTROL_LAYOUT);
    control_query=0x41fcd0; control_setter=0x91aa90;
    CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123410,&control)==COOP_PLAYER_CONTROL_LAYOUT);
    control_setter=0x438230;
    CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x123510,&control)==COOP_PLAYER_CONTROL_LAYOUT);
    CHECK(CoopPlayerControlObserve(control_read,NULL,0x1000,0x12340e,&control)==COOP_PLAYER_CONTROL_LAYOUT);
    CHECK(CoopPlayerControlObserve(control_read,NULL,UINT32_MAX-1,0x123410,&control)==COOP_PLAYER_CONTROL_UNREADABLE);
    /* Ordinary owned storage avoids conflicting with an existing mapping.
       The compile-time fixture seam is absent from the actual game DLL. */
    game_pointer=(uintptr_t)game;
    game[0x118/4]=(uint32_t)(uintptr_t)player;
    player[0]=(uint32_t)(uintptr_t)player_table;
    player[0x190/4]=(uint32_t)(uintptr_t)physics;
    player[0x60/4]=(uint32_t)(uintptr_t)model; model[0]=0x191c850;
    float axis=1; memcpy((char *)model+0x3c,&axis,4);
    player[0x1c/4]=1;
    player_table[0x8c/4]=0x938250;
    physics[0]=(uint32_t)(uintptr_t)physics_table;
    physics_table[0x54/4]=0x18ae690;
    const float xyz[3]={1,2,3};
    memcpy((char *)physics+0x1d8,xyz,sizeof(xyz));
    CHECK(CoopTelemetryStart(L"fixture-no-game-evidence.log")==0);
    CHECK(tick_expects(0));
    CoopTelemetryEvent(10); CoopTelemetryEvent(13);
    CoopTelemetryEvent(4); CHECK(tick_expects(0)); /* Unknown paused frame fails closed. */
    CoopTelemetryEvent(35);
    CHECK(tick_expects(0));  /* Objects exist but loading is unfinished. */
    CoopTelemetryEvent(33); CHECK(tick_expects(1)); CHECK(published_epoch==1);
    CHECK(facing_valid && facing==0);
    axis=0; memcpy((char *)model+0x3c,&axis,4);
    axis=1; memcpy((char *)model+0x4c,&axis,4);
    CHECK(tick_expects(1)); CHECK(facing_valid && fabsf(facing-1.5707963f)<0.00001f);
    axis=NAN; memcpy((char *)model+0x4c,&axis,4);
    CHECK(tick_expects(1)); CHECK(!facing_valid && facing==0);
    axis=0; memcpy((char *)model+0x4c,&axis,4);
    CHECK(tick_expects(1)); CHECK(!facing_valid); /* Degenerate basis. */
    int before_pause=publications;
    now+=49; CoopTelemetryEvent(3); CHECK(publications==before_pause);
    now+=1; CoopTelemetryEvent(3); CHECK(publications==before_pause+1);
    before_pause=publications;
    CoopTelemetryEvent(34);
    CHECK(active==0 && publications==before_pause+1); /* No stall timeout. */
    CHECK(tick_expects(0)); /* A stray ordinary tick must not undo pause. */
    CoopTelemetryEvent(4); CHECK(tick_expects(0)); /* Paused lifecycle remains authoritative. */
    CoopTelemetryEvent(35); CHECK(tick_expects(1));
    CoopTelemetryEvent(4); CHECK(tick_expects(1)); /* Late paused frame after explicit resume. */
    CoopTelemetryEvent(35); CHECK(tick_expects(1));
    CoopTelemetryEvent(32); CHECK(tick_expects(0));
    CoopTelemetryEvent(33); CHECK(tick_expects(1));
    player_table[0x8c/4]=0; CHECK(tick_expects(0));
    player_table[0x8c/4]=0x938250; CHECK(tick_expects(1));
    CoopTelemetryEvent(11); CHECK(tick_expects(0));
    CoopTelemetryEvent(12); CoopTelemetryEvent(10);
    CoopTelemetryEvent(33); CHECK(tick_expects(0)); /* Init also required. */
    CoopTelemetryEvent(13); CHECK(tick_expects(1)); CHECK(published_epoch==2);
    CoopTelemetryEvent(14); CHECK(tick_expects(0));
    CoopTelemetryEvent(39); CHECK(stopped && active==0);
    puts("PASS production adapter fixture: loading, pause, invalid reader, close, second epoch and shutdown");
    return 0;
}
