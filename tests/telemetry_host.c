/* Isolated memory fixture for the production read-only adapter. This is not
   Mafia II, a simulated game client, or evidence of gameplay. No game code is
   loaded: numeric vtable targets are checked as data and never executed. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/game/telemetry.h"

static uint32_t game[0x120/4], player[0x198/4], player_table[0x90/4];
static uint32_t physics[0x1e8/4], physics_table[0x58/4];
static int active, publications, stopped;
static uint32_t published_epoch;

int CoopStoryTestStart(const wchar_t *path) {(void)path; return 0;}
void CoopStoryTestEvent(unsigned int event) {(void)event;}
int CoopNetworkStart(const wchar_t *path) {(void)path; return 0;}
void CoopNetworkStop(void) {stopped=1;}
void CoopNetworkPublish(uint32_t epoch,const float *position) {
    published_epoch=epoch; active=position!=NULL; ++publications;
    if(position && (position[0]!=1 || position[1]!=2 || position[2]!=3)) active=-1;
}
static int tick_expects(int expected) {
    Sleep(55); CoopTelemetryEvent(3); return active==expected;
}
#define CHECK(test) do {if(!(test)){fprintf(stderr,"FAIL line %d\n",__LINE__);return 1;}} while(0)
int main(void) {
    /* Map only the singleton pointer page in this owned x86 fixture process. */
    void *page=VirtualAlloc((void *)(uintptr_t)0x1ab0000,0x10000,
                           MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    CHECK(page==(void *)(uintptr_t)0x1ab0000);
    *(uintptr_t *)(uintptr_t)0x1abfe14=(uintptr_t)game;
    game[0x118/4]=(uint32_t)(uintptr_t)player;
    player[0]=(uint32_t)(uintptr_t)player_table;
    player[0x190/4]=(uint32_t)(uintptr_t)physics;
    player[0x1c/4]=1;
    player_table[0x8c/4]=0x938250;
    physics[0]=(uint32_t)(uintptr_t)physics_table;
    physics_table[0x54/4]=0x18ae690;
    const float xyz[3]={1,2,3};
    memcpy((char *)physics+0x1d8,xyz,sizeof(xyz));
    CHECK(CoopTelemetryStart(L"fixture-no-game-evidence.log")==0);
    CHECK(tick_expects(0));
    CoopTelemetryEvent(10); CoopTelemetryEvent(13);
    CHECK(tick_expects(0));  /* Objects exist but loading is unfinished. */
    CoopTelemetryEvent(33); CHECK(tick_expects(1)); CHECK(published_epoch==1);
    int before_pause=publications;
    CoopTelemetryEvent(34);
    CHECK(active==0 && publications==before_pause+1); /* No stall timeout. */
    CoopTelemetryEvent(35); CHECK(tick_expects(1));
    CoopTelemetryEvent(4); CHECK(active==0); CHECK(tick_expects(1));
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
    CHECK(VirtualFree(page,0,MEM_RELEASE));
    puts("PASS production adapter fixture: loading, pause, invalid reader, close, second epoch and shutdown");
    return 0;
}
