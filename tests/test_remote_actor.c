/* Owned callback fixture only. Does not access Mafia or execute engine code. */
#include "../src/game/remote_actor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    unsigned int created, moved, stopped, removed;
    int gone, refuse_stop, fail_create;
    float position[3], velocity[2];
    unsigned int faced;
    int refuse_face;
    float heading;
} Backend;
static uint32_t create(void *context, const float p[3]) {
    Backend *b = context; ++b->created;
    if (b->fail_create) return 0;
    memcpy(b->position, p, sizeof(b->position)); return 123;
}
static int position(void *context, uint32_t handle, float p[3]) {
    Backend *b = context; assert(handle == 123);
    memcpy(p, b->position, sizeof(b->position)); return 1;
}
static int move(void *context, uint32_t handle, const float v[2]) {
    Backend *b = context; assert(handle == 123); ++b->moved;
    memcpy(b->velocity, v, sizeof(b->velocity)); return 1;
}
static int stop(void *context, uint32_t handle) {
    Backend *b = context; assert(handle == 123); ++b->stopped; return !b->refuse_stop;
}
static int remove_actor(void *context, uint32_t handle) {
    Backend *b = context; assert(handle == 123); ++b->removed; return 1;
}
static int gone(void *context, uint32_t handle) {
    Backend *b = context; assert(handle == 123); return b->gone;
}
static CoopRemoteSnapshot snapshot(void) {
    CoopRemoteSnapshot s = {0};
    s.ready = s.active = s.have_state = s.world_authorized = 1;
    s.player_id = 2; s.epoch = s.connection_generation = s.world_generation = 1;
    s.session_id = 9; s.state_received_ms = s.mailbox_published_ms = s.world_observed_ms = 1000;
    return s;
}
static int face(void *context, uint32_t handle, float heading) {
    Backend *b=context; assert(handle==123); ++b->faced; b->heading=heading;
    return !b->refuse_face;
}
int main(void) {
    Backend b = {0}; CoopRemoteActor a;
    CoopRemoteActorOps ops = {create, position, move, stop, remove_actor, gone, face};
    CoopRemoteSnapshot s = snapshot();
    CoopRemoteActorInit(&a, NULL, NULL);
    assert(CoopRemoteActorTick(&a, &s, 1000) == COOP_ACTOR_DISABLED);
    CoopRemoteActorInit(&a, &ops, &b);
    s.visual_only=1;
    assert(CoopRemoteActorTick(&a,&s,1000)==COOP_ACTOR_WAIT_WORLD && !b.created);
    s.visual_only=0;
    s.world_authorized = 0;
    assert(CoopRemoteActorTick(&a, &s, 1000) == COOP_ACTOR_WAIT_WORLD && !b.created);
    s.world_authorized = 1;
    assert(CoopRemoteActorTick(&a, &s, 1000) == COOP_ACTOR_CREATING && b.created == 1 && !b.moved);
    s.position[0] = 2;
    assert(CoopRemoteActorTick(&a, &s, 1001) == COOP_ACTOR_MOVING);
    assert(fabsf(b.velocity[0] - 4) < 0.001f && b.velocity[1] == 0);
    s.position[0] = 0;
    assert(CoopRemoteActorTick(&a, &s, 1002) == COOP_ACTOR_STANDING && b.stopped == 1);
    s.position[0] = 0.15f;
    assert(CoopRemoteActorTick(&a, &s, 1003) == COOP_ACTOR_STANDING && b.stopped == 1);
    s.position[0] = 30;
    assert(CoopRemoteActorTick(&a, &s, 1004) == COOP_ACTOR_TOO_FAR && b.created == 1);
    ++s.world_generation;
    assert(CoopRemoteActorTick(&a, &s, 1005) == COOP_ACTOR_REMOVING && b.removed == 1);
    assert(CoopRemoteActorTick(&a, &s, 1006) == COOP_ACTOR_REMOVING && b.removed == 1 && b.created == 1);
    b.gone = 1;
    assert(CoopRemoteActorTick(&a, &s, 1500) == COOP_ACTOR_WAIT_WORLD && !a.handle);
    s.position[0] = 0;
    assert(CoopRemoteActorTick(&a, &s, 1501) == COOP_ACTOR_CREATING && b.created == 2);
    b.refuse_stop = 1;
    assert(CoopRemoteActorTick(&a, NULL, 1502) == COOP_ACTOR_BACKEND_REFUSED && b.removed == 1);
    b.refuse_stop = 0; b.gone = 0;
    assert(CoopRemoteActorTick(&a, NULL, 1503) == COOP_ACTOR_REMOVING && b.removed == 2);
    b.gone = 1; CoopRemoteActorTick(&a, NULL, 1504); assert(!a.handle);
    s = snapshot(); s.position[0] = NAN;
    assert(CoopRemoteActorTick(&a, &s, 1600) == COOP_ACTOR_WAIT_WORLD && b.created == 2);
    s = snapshot();
    assert(CoopRemoteActorTick(&a, &s, 3000) == COOP_ACTOR_WAIT_WORLD && b.created == 2);
    b.fail_create = 1;
    s.mailbox_published_ms = s.state_received_ms = s.world_observed_ms = 3500;
    assert(CoopRemoteActorTick(&a, &s, 3500) == COOP_ACTOR_BACKEND_REFUSED);
    unsigned int attempts = b.created;
    assert(CoopRemoteActorTick(&a, &s, 3501) == COOP_ACTOR_CREATING && b.created == attempts);
    Backend turning={0}; CoopRemoteActor turn_actor;
    CoopRemoteActorInit(&turn_actor,&ops,&turning);
    s=snapshot(); s.heading_valid=1; s.heading=1.25f;
    assert(CoopRemoteActorTick(&turn_actor,&s,1000)==COOP_ACTOR_CREATING && !turning.faced);
    assert(CoopRemoteActorTick(&turn_actor,&s,1001)==COOP_ACTOR_STANDING);
    assert(turning.faced==1 && turning.heading==1.25f && !turning.moved);
    s.heading=0; /* Valid zero radians still requests an in-place turn. */
    assert(CoopRemoteActorTick(&turn_actor,&s,1002)==COOP_ACTOR_STANDING);
    assert(turning.faced==2 && turning.heading==0);
    s.heading_valid=0;
    assert(CoopRemoteActorTick(&turn_actor,&s,1003)==COOP_ACTOR_STANDING && turning.faced==2);
    s.heading_valid=1; turning.refuse_face=1;
    assert(CoopRemoteActorTick(&turn_actor,&s,1004)==COOP_ACTOR_REMOVING);
    assert(turning.removed==1 && !turning.moved);
    assert(CoopRemoteActorTick(&turn_actor,NULL,1005)==COOP_ACTOR_REMOVING && turning.faced==3);
    turning.gone=1; CoopRemoteActorTick(&turn_actor,NULL,1006); assert(!turn_actor.handle);
    {
        Backend smooth={0}; CoopRemoteActor actor;
        CoopRemoteActorInit(&actor,&ops,&smooth);
        assert(CoopRemoteActorEnableSmoothing(&actor,1));
        s=snapshot(); s.received_sequence=1; s.heading_valid=1; s.heading=0;
        assert(CoopRemoteActorTick(&actor,&s,1000)==COOP_ACTOR_CREATING);
        s.received_sequence=2; s.heading=1;
        s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=1050;
        assert(CoopRemoteActorTick(&actor,&s,1050)==COOP_ACTOR_STANDING);
        assert(smooth.faced==1 && fabsf(smooth.heading)<0.001f);
        assert(CoopRemoteActorTick(&actor,&s,1125)==COOP_ACTOR_STANDING);
        assert(smooth.faced==2 && fabsf(smooth.heading-0.5f)<0.001f);
        assert(s.heading==1 && s.state_received_ms==1050); /* Authority stays raw. */
        s.received_sequence=3; s.heading_valid=0; s.heading=0;
        s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=1150;
        assert(CoopRemoteActorTick(&actor,&s,1175)==COOP_ACTOR_STANDING);
        assert(smooth.faced==2); /* Do not invent a facing across unknown pose. */
    }
    {
        Backend guarded={0}; CoopRemoteActor actor;
        CoopRemoteActorInit(&actor,&ops,&guarded);
        assert(CoopRemoteActorEnableSmoothing(&actor,1));
        s=snapshot(); s.received_sequence=1;
        assert(CoopRemoteActorTick(&actor,&s,1000)==COOP_ACTOR_CREATING);
        s.received_sequence=2; s.position[0]=19;
        s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=1050;
        assert(CoopRemoteActorTick(&actor,&s,1050)==COOP_ACTOR_STANDING);
        s.received_sequence=3; s.position[0]=22;
        s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=1100;
        assert(CoopRemoteActorTick(&actor,&s,1100)==COOP_ACTOR_TOO_FAR);
        assert(!guarded.moved); /* Delayed target is near; raw position must veto. */
    }
    {
        Backend rejected={0}; CoopRemoteActor actor;
        CoopRemoteActorInit(&actor,&ops,&rejected);
        assert(CoopRemoteActorEnableSmoothing(&actor,1));
        s=snapshot(); s.received_sequence=1;
        assert(CoopRemoteActorTick(&actor,&s,1000)==COOP_ACTOR_CREATING);
        s.received_sequence=3;
        s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=1050;
        assert(CoopRemoteActorTick(&actor,&s,1050)==COOP_ACTOR_STANDING);
        s.received_sequence=2;
        assert(CoopRemoteActorTick(&actor,&s,1051)==COOP_ACTOR_REMOVING);
        rejected.gone=1;
        assert(CoopRemoteActorTick(&actor,&s,1052)==COOP_ACTOR_WAIT_WORLD);
        for(uint64_t now=1053;now<1060;++now)
            assert(CoopRemoteActorTick(&actor,&s,now)==COOP_ACTOR_WAIT_WORLD);
        assert(rejected.created==1 && rejected.removed==1);
        assert(actor.timeline.have_newest && actor.timeline.newest.received_sequence==3);
    }
    puts("PASS actor retirement, raw authority guards, delayed heading, unknown pose and rejected mailbox replay");
    return 0;
}
