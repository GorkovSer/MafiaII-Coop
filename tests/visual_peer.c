/* Explicit synthetic guest trajectory for one-game visual acceptance only.
   Never presented as a second game client or two-PC gameplay acceptance. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/net/session.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
    if(argc!=9 || strlen(argv[4])!=64) return 2;
    CoopSessionConfig config={0};
    config.role=!strcmp(argv[1],"host") ? COOP_HOST : !strcmp(argv[1],"guest") ? COOP_GUEST : 0;
    config.address=argv[2];
    unsigned long port=strtoul(argv[3],NULL,10),duration=strtoul(argv[5],NULL,10);
    if(!port || port>65535 || duration<3000 || duration>600000) return 2;
    config.port=(uint16_t)port; config.capabilities=COOP_CAP_TELEMETRY|COOP_CAP_VISUAL_SCENE;
    config.module_version=COOP_MODULE_VERSION; memcpy(config.build,CoopGameBuild,32);
    unsigned char key[32];
    for(unsigned int i=0;i<32;++i) {
        char pair[3]={argv[4][2*i],argv[4][2*i+1],0},*end;
        unsigned long value=strtoul(pair,&end,16); if(*end || value>255) return 2;
        key[i]=(unsigned char)value;
    }
    if(!CoopWorldKeyValid(key)) return 2;
    float origin[3];
    for(unsigned int i=0;i<3;++i) {origin[i]=strtof(argv[6+i],NULL); if(!isfinite(origin[i]) || fabsf(origin[i])>=1000000) return 2;}
    CoopSession s; if(CoopSessionStart(&s,&config,NULL,NULL)) return 3;
    ULONGLONG begin=GetTickCount64(); int restarted=0,was_ready=0;
    unsigned int connections=0;
    while(GetTickCount64()-begin<duration) {
        ULONGLONG elapsed=GetTickCount64()-begin;
        if(config.role==COOP_GUEST && elapsed>=18000 && !restarted) {
            connections+=s.connections; CoopSessionStop(&s); Sleep(750);
            if(CoopSessionStart(&s,&config,NULL,NULL)) return 4;
            restarted=1; was_ready=0;
        }
        float t=(float)((elapsed>=3000 ? elapsed-3000 : 0)%15000)/1000.0f;
        float p[3]={origin[0],origin[1],origin[2]},heading=t<2 ? 0 : 1.5707963f;
        p[0]+=fminf(t,2)*.75f;
        if(t>=4) p[1]+=fminf(t-4,2)*.75f;
        CoopSessionWorld(&s,1,key);
        CoopSessionPublishPose(&s,1,elapsed>=3000 ? p : NULL,&heading);
        /* Stall app states but keep ENet serviced: tests real receiver expiry. */
        if(elapsed>=9000 && elapsed<12000) s.next_send=enet_time_get()+60000;
        else if(s.next_send>enet_time_get()+1000) s.next_send=enet_time_get();
        CoopSessionTick(&s);
        if(was_ready && !s.ready) break; /* Normal server/game shutdown. */
        was_ready=s.ready; Sleep(5);
    }
    connections+=s.connections;
    printf("synthetic_visual_peer=1 connections=%u sent=%lu received=%lu\n",connections,
        (unsigned long)s.sent_states,(unsigned long)s.received_states);
    CoopSessionStop(&s); return 0;
}
