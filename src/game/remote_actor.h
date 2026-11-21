#ifndef COOP_REMOTE_ACTOR_H
#define COOP_REMOTE_ACTOR_H
#include "../net/game_bridge.h"

/* Game-thread-only controller. Backend owns all engine references and must
   validate pinned image, registry identity, context and AI exclusion itself. */
typedef struct {
    /* Zero means no retained resources; nonzero transfers exactly one handle. */
    uint32_t (*create)(void *context, const float position[3]);
    int (*position)(void *context, uint32_t handle, float out[3]);
    int (*move)(void *context, uint32_t handle, const float velocity_xy[2]);
    int (*stop)(void *context, uint32_t handle);
    int (*begin_remove)(void *context, uint32_t handle);
    int (*removed)(void *context, uint32_t handle); /* registry absence, not queued release */
} CoopRemoteActorOps;
typedef enum {
    COOP_ACTOR_DISABLED, COOP_ACTOR_WAIT_WORLD, COOP_ACTOR_CREATING,
    COOP_ACTOR_MOVING, COOP_ACTOR_STANDING, COOP_ACTOR_REMOVING,
    COOP_ACTOR_BACKEND_REFUSED, COOP_ACTOR_TOO_FAR
} CoopRemoteActorStatus;
typedef struct {
    CoopRemoteActorOps ops;
    void *context;
    uint32_t handle, retiring, stopped, removal_started;
    uint64_t session, retry_after;
    uint32_t connection, generation, epoch, player;
    CoopRemoteActorStatus status;
} CoopRemoteActor;
void CoopRemoteActorInit(CoopRemoteActor *actor, const CoopRemoteActorOps *ops, void *context);
/* NULL/expired/unagreed remote retires the owned actor. Repeat while removal
   is pending, including in menus. Never reset a live controller to discard it. */
CoopRemoteActorStatus CoopRemoteActorTick(CoopRemoteActor *actor,
    const CoopRemoteSnapshot *remote, uint64_t now_ms);
#endif
