/* Exercises the thread/mailbox bridge without loading or emulating Mafia II. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/net/game_bridge.h"
#include <stdio.h>
#include <stdlib.h>
int wmain(int argc,wchar_t **argv) {
    if(argc!=3) return 2;
    if(CoopNetworkStart(argv[1])) return 3;
    unsigned long duration=wcstoul(argv[2],NULL,10);
    if(duration<3000 || duration>20000) {CoopNetworkStop();return 2;}
    ULONGLONG start=GetTickCount64();
    int saw_active=0,saw_inactive=0,ready=0;
    CoopRemoteSnapshot remote={0};
    float fixture[3]={12.25f,-8.0f,4.0f};
    while(GetTickCount64()-start<duration) {
        ULONGLONG elapsed=GetTickCount64()-start;
        /* Stop publishing halfway through; the network must keep servicing
           while the stale game snapshot is invalidated independently. */
        if(elapsed<1000) CoopNetworkPublish(1,fixture);
        if(CoopNetworkReadRemote(&remote)) {
            ready=1;
            if(remote.active && remote.position[0]==fixture[0]) saw_active=1;
            if(saw_active && !remote.active) saw_inactive=1;
        }
        Sleep(10);
    }
    printf("bridge_fixture=1 ready=%d saw_active=%d saw_inactive=%d received=%lu\n",
        ready,saw_active,saw_inactive,(unsigned long)remote.received_states);
    CoopNetworkStop();
    return 0;
}
