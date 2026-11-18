/* Two-process ENet integration fixture. Positions are test vectors, not
   game observations. This executable is not the user-facing mod. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/net/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void log_line(void *context, const char *event) {
    (void)context; printf("event=%s\n",event); fflush(stdout);
}
int main(int argc,char **argv) {
    if(argc<5 || argc>6) return 2;
    CoopSessionConfig config={0};
    config.role=!strcmp(argv[1],"host")?COOP_HOST:!strcmp(argv[1],"guest")?COOP_GUEST:0;
    config.address=argv[2];
    char *end=NULL; unsigned long port=strtoul(argv[3],&end,10);
    if(!*argv[3] || *end || port<1 || port>65535) return 2;
    config.port=(uint16_t)port; config.capabilities=COOP_CAP_TELEMETRY;
    config.module_version=COOP_MODULE_VERSION;
    memcpy(config.build,CoopGameBuild,32);
    int mode=argc==6 ? atoi(argv[5]):0;
    if(mode==1) config.build[0]^=1;
    if(mode==2) config.capabilities^=2;
    if(mode==3) ++config.module_version;
    CoopSession session;
    int started=CoopSessionStart(&session,&config,log_line,NULL);
    if(started) {printf("start_error=%d\n",started);return 3;}
    float xyz[3]={config.role==COOP_HOST?10.25f:-4.0f,20.5f,3.0f};
    CoopSessionPublish(&session,1,xyz);
    ULONGLONG start=GetTickCount64(), duration=(ULONGLONG)strtoul(argv[4],NULL,10);
    if(duration>30000) {CoopSessionStop(&session);return 2;}
    int cleared=0, remote_fixture_seen=0, expired_while_ready=0;
    while(GetTickCount64()-start<duration) {
        ULONGLONG elapsed = GetTickCount64()-start;
        if(mode==4 && elapsed>1000) {
            /* Service ENet/control traffic but deliberately send no more app state. */
            session.next_send=enet_time_get()+60000;
        } else if(!cleared && elapsed>duration/2) {CoopSessionPublish(&session,1,NULL);cleared=1;}
        CoopSessionTick(&session);
        if(session.remote_active && session.remote.position[0]==(config.role==COOP_HOST?-4.0f:10.25f)) remote_fixture_seen=1;
        if(remote_fixture_seen && session.ready && !session.remote_active && session.have_sequence &&
            CoopSessionRemoteAge(&session,enet_time_get())>=COOP_STATE_MAX_AGE_MS &&
            !session.remote.active && !session.remote.position[0] && !session.remote.position[1] && !session.remote.position[2])
            expired_while_ready=1;
        Sleep(5);
    }
    printf("transport_fixture=1 connections=%lu sent=%lu received=%lu rejected=%lu remote_fixture_seen=%d remote_active=%d\n",
        (unsigned long)session.connections,(unsigned long)session.sent_states,(unsigned long)session.received_states,
        (unsigned long)session.rejected,remote_fixture_seen,session.remote_active);
    CoopSessionStop(&session);
    printf("remote_expired_while_ready=%d\n",expired_while_ready);
    return 0;
}
