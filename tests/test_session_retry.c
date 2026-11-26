/* Real loopback transport with synthetic poses; not two-game acceptance. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../src/net/session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned int retries;
static void event(void *context,const char *name) {
    (void)context;
    if(!strcmp(name,"retry_connecting")) ++retries;
}
static CoopSessionConfig config(int role,unsigned int port,int retry) {
    CoopSessionConfig c={0}; c.role=role; c.address="127.0.0.1";
    c.port=(uint16_t)port; c.auto_reconnect=retry;
    c.capabilities=COOP_CAP_TELEMETRY|COOP_CAP_VISUAL_SCENE;
    c.module_version=COOP_MODULE_VERSION; memcpy(c.build,CoopGameBuild,32); return c;
}
static void step(CoopSession *s) {
    if(!s || !s->host) return;
    unsigned char key[32]={1}; float p[3]={s->role==COOP_HOST ? 10.f : -4.f,20,3},heading=.5f;
    CoopSessionWorld(s,1,key); CoopSessionPublishPose(s,1,p,&heading); CoopSessionTick(s);
}
static int wait_state(CoopSession *host,CoopSession *guest,int state,unsigned int ms) {
    ULONGLONG until=GetTickCount64()+ms;
    do {
        step(host); step(guest);
        if(state==1 && guest->ready && guest->remote_active &&
           CoopWorldAccepts(&guest->world,guest->remote.world.generation,guest->remote.epoch)) return 1;
        if(state==2 && !guest->peer) return 1;
        if(state==3 && guest->retry_blocked && !guest->peer) return 1;
        Sleep(5);
    } while(GetTickCount64()<until);
    return 0;
}
#define CHECK(test,label) do {if(!(test)) {fprintf(stderr,"FAIL retry: %s\n",label); goto done;}} while(0)
int main(int argc,char **argv) {
    if(argc!=2) return 2;
    char *end; unsigned long port=strtoul(argv[1],&end,10);
    if(*end || port<1024 || port>=65535) return 2;
    CoopSession host={0},guest={0},passive={0},invalid={0}; int result=1;
    CoopSessionConfig hc=config(COOP_HOST,(unsigned int)port,0);
    CoopSessionConfig gc=config(COOP_GUEST,(unsigned int)port,1);
    CoopSessionConfig pc=config(COOP_GUEST,(unsigned int)port+1,0);
    CoopSessionConfig bad=hc; bad.auto_reconnect=1;
    CHECK(CoopSessionStart(&invalid,&bad,NULL,NULL)==1,"host retry config refused");
    bad=gc; bad.auto_reconnect=2;
    CHECK(CoopSessionStart(&invalid,&bad,NULL,NULL)==1,"invalid retry flag refused");
    CHECK(!CoopSessionStart(&guest,&gc,event,NULL),"auto guest starts without host");
    CHECK(!CoopSessionStart(&passive,&pc,NULL,NULL),"default guest starts");
    ULONGLONG begin=GetTickCount64();
    while(GetTickCount64()-begin<5500) {step(&guest);step(&passive);Sleep(5);}
    CHECK(!guest.peer && guest.retry_pending && !guest.have_sequence,"initial timeout schedules retry");
    CHECK(!passive.peer && !passive.retry_pending,"default policy does not retry");
    CHECK(!CoopSessionStart(&host,&hc,NULL,NULL),"late host starts");
    CHECK(wait_state(&host,&guest,1,5000),"late host connects with fresh world");
    CHECK(guest.connections==1 && guest.remote.heading_valid && guest.remote.position[0]==10.f,"first pose accepted");
    uint64_t old_session=guest.session;
    CoopSessionStop(&host);
    CHECK(wait_state(NULL,&guest,2,3000),"disconnect observed");
    CHECK(!guest.ready && !guest.remote_active && !guest.have_sequence && !guest.world.ready &&
          !guest.session && !guest.player_id && !guest.remote.position[0] && !guest.remote.heading_valid,
          "disconnect clears identity, world and old pose");
    CHECK(!CoopSessionStart(&host,&hc,NULL,NULL),"replacement host starts");
    CHECK(wait_state(&host,&guest,1,5000),"replacement host reconnects");
    CHECK(guest.connections==2 && guest.session!=old_session,"new session identity and connection generation");
    CoopSessionStop(&host);
    CHECK(wait_state(NULL,&guest,2,3000),"second disconnect observed");
    ++hc.module_version;
    CHECK(!CoopSessionStart(&host,&hc,NULL,NULL),"incompatible host starts");
    CHECK(wait_state(&host,&guest,3,5000),"observed protocol refusal blocks retry");
    unsigned int before=retries;
    begin=GetTickCount64();
    while(GetTickCount64()-begin<2300) {step(&host);step(&guest);Sleep(5);}
    CHECK(retries==before && !guest.peer && !guest.retry_pending && !guest.ready,"refusal stays blocked");
    CoopSessionStop(&guest); CoopSessionTick(&guest);
    CHECK(!guest.host && !guest.retry_pending,"stop cannot reconnect");
    puts("PASS guest retry: late host, replacement host, stale identity/pose removal, refusal, default-off, stop");
    result=0;
done:
    CoopSessionStop(&host);CoopSessionStop(&guest);CoopSessionStop(&passive);return result;
}
