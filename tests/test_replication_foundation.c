#include "../src/net/remote_timeline.h"
#include "../src/game/entity_map.h"
#include "../src/game/remote_actor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static CoopRemoteSnapshot sample(uint64_t ms, uint32_t seq, float x) {
    CoopRemoteSnapshot s={0};
    s.ready=s.active=s.have_state=s.world_authorized=1;
    s.session_id=9; s.player_id=2;
    s.epoch=s.connection_generation=s.world_generation=1;
    s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=ms;
    s.received_sequence=seq; s.position[0]=x; return s;
}
static CoopEntityStatus absent(void *context, uint32_t guid) {
    assert(guid); return *(CoopEntityStatus *)context;
}
static CoopEntityStatus reenter(void *context, uint32_t guid) {
    CoopEntityMap *m=context; CoopRemoteSnapshot s=sample(1000,1,0);
    assert(guid && !CoopEntityMapSynchronize(m,&s,1000));
    assert(!CoopEntityMapRetire(m,1)); return COOP_ENTITY_ABSENT;
}
static uint32_t create(void *c,const float p[3]) { (void)c; (void)p; return 123; }
static int position(void *c,uint32_t id,float p[3]) { (void)c; assert(id==123); memset(p,0,12); return 1; }
static int move(void *c,uint32_t id,const float v[2]) { (void)c; assert(id==123 && v[0]>0); return 1; }
static int yes(void *c,uint32_t id) { (void)c; assert(id==123); return 1; }
int main(void) {
    {
        CoopRemoteTimeline pose_timeline={0}; CoopRemoteFrame frame;
        CoopRemoteSnapshot pose=sample(100,1,0);
        pose.heading_valid=1; pose.heading=1;
        assert(CoopRemoteTimelineAt(&pose_timeline,&pose,100,&frame));
        assert(frame.authority.heading_valid && frame.authority.heading==1);
        pose.heading=2; /* Same packet sequence cannot acquire a new facing. */
        assert(!CoopRemoteTimelineAt(&pose_timeline,&pose,101,&frame));
        assert(!pose_timeline.count && !frame.authority.ready);
    }
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,1,0);
    assert(CoopRemoteTimelineAt(&t,&s,1000,&f) && f.position[0]==0);
    s=sample(1050,2,10);
    assert(CoopRemoteTimelineAt(&t,&s,1050,&f) && f.position[0]==0);
    assert(CoopRemoteTimelineAt(&t,&s,1125,&f) && f.interpolated && fabsf(f.position[0]-5)<0.001f);
    assert(f.authority.position[0]==10 && f.authority.state_received_ms==1050);
    s=sample(1130,3,100); s.world_generation=2;
    assert(CoopRemoteTimelineAt(&t,&s,1130,&f) && t.count==1 && f.position[0]==100);
    s.world_authorized=0;
    assert(!CoopRemoteTimelineAt(&t,&s,1131,&f) && !t.count && !f.authority.ready);
    s=sample(1150,3,20); s.world_generation=2;
    assert(CoopRemoteTimelineAt(&t,&s,1150,&f) && f.position[0]==20);
    s.received_sequence=4; s.position[0]=30;
    assert(CoopRemoteTimelineAt(&t,&s,1150,&f) && t.count==1 && f.position[0]==30);
    assert(CoopRemoteTimelineAt(&t,&f.authority,1151,&f) && f.authority.ready);
    s=sample(1600,5,40); s.world_generation=2;
    assert(CoopRemoteTimelineAt(&t,&s,1600,&f) && t.count==1);
    assert(CoopRemoteTimelineAt(&t,&s,1750,&f) && f.position[0]==40 && !f.interpolated);
    assert(!CoopRemoteTimelineAt(&t,&s,3600,&f) && !t.count);
    s=sample(4000,UINT32_MAX,0);
    assert(CoopRemoteTimelineAt(&t,&s,4000,&f));
    s=sample(4050,0,1); assert(CoopRemoteTimelineAt(&t,&s,4050,&f) && t.count==2);
    s=sample(4060,UINT32_MAX,2); assert(!CoopRemoteTimelineAt(&t,&s,4060,&f));
    s=sample(4100,1,NAN); assert(!CoopRemoteTimelineAt(&t,&s,4100,&f));

    CoopRemoteActor a; CoopRemoteActorOps ops={create,position,move,yes,yes,yes,NULL};
    CoopRemoteActorInit(&a,&ops,NULL); assert(CoopRemoteActorEnableSmoothing(&a,1));
    s=sample(1000,1,0); assert(CoopRemoteActorTick(&a,&s,1000)==COOP_ACTOR_CREATING);
    assert(!CoopRemoteActorEnableSmoothing(&a,0));
    s=sample(1050,2,10); assert(CoopRemoteActorTick(&a,&s,1050)==COOP_ACTOR_STANDING);
    assert(CoopRemoteActorTick(&a,&s,1125)==COOP_ACTOR_MOVING);
    s=sample(1130,3,100); assert(CoopRemoteActorTick(&a,&s,1130)==COOP_ACTOR_TOO_FAR);
    assert(CoopRemoteActorTick(&a,NULL,1131)==COOP_ACTOR_WAIT_WORLD && !a.handle);

    CoopEntityIdSource ids={0};
    assert(!CoopEntityIdSourceStart(&ids,9,2));
    assert(CoopEntityIdSourceStart(&ids,9,1) && CoopEntityIdNext(&ids,9)==1);
    assert(CoopEntityIdSourceStart(&ids,9,1) && CoopEntityIdNext(&ids,9)==2);
    assert(!CoopEntityIdNext(&ids,10));
    ids.next_id=UINT64_MAX; assert(CoopEntityIdNext(&ids,9)==UINT64_MAX);
    assert(!CoopEntityIdSourceStart(&ids,9,1) && !CoopEntityIdNext(&ids,9));
    CoopEntityMap m={0}; CoopEntityBinding b; s=sample(1000,1,0);
    assert(!CoopEntityMapBind(&m,&s,1000,1,COOP_NET_NPC,14));
    assert(!CoopEntityMapBind(&m,&s,1000,1,COOP_NET_VEHICLE,14));
    assert(CoopEntityMapBind(&m,&s,1000,1,COOP_NET_PLAYER,14));
    assert(CoopEntityMapBind(&m,&s,1000,1,COOP_NET_PLAYER,14) && m.count==1);
    assert(!CoopEntityMapBind(&m,&s,1000,1,COOP_NET_PLAYER,15));
    assert(!CoopEntityMapBind(&m,&s,1000,2,COOP_NET_PLAYER,14));
    assert(CoopEntityMapFind(&m,&s,1000,1,&b) && b.guid==14);
    s.world_authorized=0; assert(!CoopEntityMapFind(&m,&s,1001,1,&b) && !b.guid);
    s=sample(1002,2,0); ++s.world_generation;
    assert(!CoopEntityMapBind(&m,&s,1002,2,COOP_NET_PLAYER,15));
    CoopEntityStatus status=COOP_ENTITY_INVALID;
    assert(!CoopEntityMapConfirmRemoved(&m,1,absent,&status) && m.count==1);
    status=COOP_ENTITY_GUID_MISMATCH;
    assert(!CoopEntityMapConfirmRemoved(&m,1,absent,&status));
    assert(CoopEntityMapFirstRetiring(&m,&b) && b.guid==14);
    assert(CoopEntityMapConfirmRemoved(&m,1,reenter,&m) && !m.count);
    assert(CoopEntityMapBind(&m,&s,1002,2,COOP_NET_PLAYER,15));
    assert(m.generation==2 && !m.quiescing);
    CoopEntityMap vehicles={0}; s=sample(1000,1,0); s.player_id=1;
    assert(CoopEntityMapBind(&vehicles,&s,1000,8,COOP_NET_VEHICLE,88));
    assert(CoopEntityMapRetire(&vehicles,8));
    status=COOP_ENTITY_PRESENT;
    assert(!CoopEntityMapConfirmRemoved(&vehicles,8,absent,&status));
    status=COOP_ENTITY_ABSENT;
    assert(CoopEntityMapConfirmRemoved(&vehicles,8,absent,&status) && !vehicles.count);
    CoopEntityMap host={0}; s=sample(1000,1,0); s.player_id=1;
    for(unsigned int i=0;i<COOP_ENTITY_MAP_CAPACITY;++i)
        assert(CoopEntityMapBind(&host,&s,1000,100+i,COOP_NET_NPC,1000+i));
    assert(!CoopEntityMapBind(&host,&s,1000,9999,COOP_NET_VEHICLE,9999));
    assert(!CoopEntityMapSynchronize(&host,NULL,1001) && host.count==COOP_ENTITY_MAP_CAPACITY);
    puts("PASS interpolation, current-world withdrawal, entity retirement and host IDs");
    return 0;
}
