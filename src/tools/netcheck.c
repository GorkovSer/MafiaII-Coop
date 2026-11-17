/* Passive diagnostic peer. Sends inactive status only; never invents a player.
   Used to inspect actual game telemetry without creating a second game world. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../net/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void log_event(void *context,const char *event) {
    (void)context; printf("network=%s\n",event); fflush(stdout);
}
int main(int argc,char **argv) {
    if(argc!=5) {puts("Usage: coop_netcheck.exe host|guest IPv4 port duration_ms");return 2;}
    CoopSessionConfig config={0};
    config.role=!strcmp(argv[1],"host")?COOP_HOST:!strcmp(argv[1],"guest")?COOP_GUEST:0;
    config.address=argv[2];
    char *end=NULL; unsigned long port=strtoul(argv[3],&end,10);
    if(!*argv[3] || *end || !port || port>65535) return 2;
    config.port=(uint16_t)port;
    unsigned long duration=strtoul(argv[4],&end,10);
    if(!*argv[4] || *end || duration<1000 || duration>3600000) return 2;
    config.capabilities=COOP_CAP_TELEMETRY; memcpy(config.build,CoopGameBuild,32);
    config.module_version=COOP_MODULE_VERSION;
    CoopSession session;
    if(CoopSessionStart(&session,&config,log_event,NULL)) return 3;
    ULONGLONG start=GetTickCount64(),next_report=start+2000;
    uint32_t last_received=0,active_samples=0;
    float low[3]={0},high[3]={0};
    while(GetTickCount64()-start<duration) {
        CoopSessionTick(&session);
        if(session.received_states!=last_received) {
            last_received=session.received_states;
            if(session.remote_active) {
                for(unsigned int i=0;i<3;++i) {
                    float value=session.remote.position[i];
                    if(!active_samples || value<low[i]) low[i]=value;
                    if(!active_samples || value>high[i]) high[i]=value;
                }
                ++active_samples;
            }
        }
        if(GetTickCount64()>=next_report) {
            printf("received=%lu active_samples=%lu rtt_ms=%lu\n",(unsigned long)session.received_states,
                (unsigned long)active_samples,(unsigned long)(session.peer?session.peer->roundTripTime:0));
            fflush(stdout); next_report=GetTickCount64()+2000;
        }
        Sleep(5);
    }
    printf("diagnostic_only=1 connections=%lu received=%lu active_samples=%lu axis_ranges=%.5f,%.5f,%.5f\n",
        (unsigned long)session.connections,(unsigned long)session.received_states,(unsigned long)active_samples,
        (double)(high[0]-low[0]),(double)(high[1]-low[1]),(double)(high[2]-low[2]));
    CoopSessionStop(&session);
    return active_samples?0:4;
}
