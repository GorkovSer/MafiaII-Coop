/* Synthetic scene/pose mailbox fixture. Does not load or call Mafia II. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/net/game_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int wmain(int argc,wchar_t **argv) {
    if(argc!=4) return 2;
    int expected=_wtoi(argv[3]);
    int result=CoopNetworkStart(argv[1]);
    if(expected==2) {printf("config_refused=%d\n",result==2); return result==2 ? 0 : 1;}
    if(result || !CoopNetworkVisualMode()) return 3;
    ULONGLONG start=GetTickCount64(),duration=(ULONGLONG)_wtoi(argv[2]);
    int ready=0,seen=0,withdrawn=0;
    float position[3]={12.25f,-8,4},heading=.75f;
    unsigned char unrelated_physical_key[32]={9};
    CoopRemoteSnapshot previous={0};
    while(GetTickCount64()-start<duration) {
        ULONGLONG elapsed=GetTickCount64()-start;
        CoopNetworkPublishVisualScene(elapsed<duration-1000 ? 1 : 0);
        /* A physical-world producer must not override a visual-only agreement. */
        CoopNetworkPublishWorld(77,unrelated_physical_key);
        CoopNetworkPublishPose(1,elapsed<duration-1000 ? position : NULL,&heading);
        CoopRemoteSnapshot physical,remote,raw,zero={0};
        memset(&physical,0xaa,sizeof(physical));
        if(CoopNetworkReadRemoteWorld(&physical) || memcmp(&physical,&zero,sizeof(zero))) return 4;
        if(CoopNetworkReadRemote(&raw) && raw.ready) ready=1;
        if(CoopNetworkReadRemoteVisual(&remote)) {
            if(!remote.active || !remote.visual_only || !remote.world_authorized || !remote.heading_valid ||
                remote.heading!=heading || remote.position[0]!=position[0]) return 5;
            seen=1;
            if(previous.session_id==remote.session_id &&
                previous.connection_generation==remote.connection_generation &&
                previous.received_sequence==remote.received_sequence &&
                previous.state_received_ms!=remote.state_received_ms) return 9;
            previous=remote;
        } else {
            if(memcmp(&remote,&zero,sizeof(zero))) return 6;
            if(seen) withdrawn=1;
        }
        Sleep(5);
    }
    CoopNetworkStop();
    CoopRemoteSnapshot stopped,zero={0}; memset(&stopped,0xaa,sizeof(stopped));
    if(CoopNetworkReadRemoteVisual(&stopped) || memcmp(&stopped,&zero,sizeof(zero))) return 7;
    printf("visual_bridge ready=%d seen=%d withdrawn=%d physical_refused=1 stable_receipt=1 stopped_zero=1\n",ready,seen,withdrawn);
    return ready && seen==expected && (!expected || withdrawn) ? 0 : 8;
}
