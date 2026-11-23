/* Exercises the thread/mailbox bridge without loading or emulating Mafia II. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/net/game_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int wmain(int argc,wchar_t **argv) {
    if(argc!=3) return 2;
    if(CoopNetworkStart(argv[1])) return 3;
    unsigned long duration=wcstoul(argv[2],NULL,10);
    if(duration<3000 || duration>20000) {CoopNetworkStop();return 2;}
    ULONGLONG start=GetTickCount64();
    int saw_active=0,saw_inactive=0,ready=0;
    uint32_t received_seen=0;
    CoopRemoteSnapshot remote={0};
    float fixture[3]={12.25f,-8.0f,4.0f};
    while(GetTickCount64()-start<duration) {
        ULONGLONG elapsed=GetTickCount64()-start;
        /* Stop publishing halfway through; the network must keep servicing
           while the stale game snapshot is invalidated independently. */
        if(elapsed<1000) CoopNetworkPublish(1,fixture);
        if(CoopNetworkReadRemote(&remote)) {
            ready=1;
            /* The earlier peer may exit before this fixture's deadline.
               ReadRemote must then zero its output, including counters. Keep
               observed receipt evidence separately from that cleared snapshot. */
            if(remote.received_states>received_seen) received_seen=remote.received_states;
            if(remote.active && remote.position[0]==fixture[0]) saw_active=1;
            if(saw_active && !remote.active) saw_inactive=1;
        }
        Sleep(10);
    }
    printf("bridge_fixture=1 ready=%d saw_active=%d saw_inactive=%d received=%lu\n",
        ready,saw_active,saw_inactive,(unsigned long)received_seen);
    CoopNetworkStop();
    memset(&remote,0xaa,sizeof(remote));
    CoopRemoteSnapshot zero={0};
    int stopped_reset=!CoopNetworkReadRemote(&remote) && !memcmp(&remote,&zero,sizeof(remote));
    printf("stopped_snapshot_zero=%d\n",stopped_reset);
    if(!stopped_reset) return 4;
    return 0;
}
