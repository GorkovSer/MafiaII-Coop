#include "remote_actor.h"
#include <math.h>
#include <string.h>

static int position_valid(const float p[3]) {
    for (unsigned int i = 0; i < 3; ++i)
        if (!isfinite(p[i]) || fabsf(p[i]) >= 1000000.0f) return 0;
    return 1;
}
static int enabled(const CoopRemoteActor *a) {
    return a->ops.create && a->ops.position && a->ops.move && a->ops.stop &&
        a->ops.begin_remove && a->ops.removed;
}
void CoopRemoteActorInit(CoopRemoteActor *a, const CoopRemoteActorOps *ops, void *context) {
    memset(a, 0, sizeof(*a));
    if (ops) a->ops = *ops;
    a->context = context; a->status = COOP_ACTOR_DISABLED;
}
static CoopRemoteActorStatus retire(CoopRemoteActor *a) {
    a->retiring = 1;
    if (!a->stopped) {
        if (!a->ops.stop(a->context, a->handle)) return a->status = COOP_ACTOR_BACKEND_REFUSED;
        a->stopped = 1;
    }
    if (!a->removal_started) {
        if (!a->ops.begin_remove(a->context, a->handle)) return a->status = COOP_ACTOR_BACKEND_REFUSED;
        a->removal_started = 1;
    }
    if (!a->ops.removed(a->context, a->handle)) return a->status = COOP_ACTOR_REMOVING;
    a->handle = a->retiring = a->stopped = a->removal_started = 0;
    return a->status = COOP_ACTOR_WAIT_WORLD;
}
CoopRemoteActorStatus CoopRemoteActorTick(CoopRemoteActor *a, const CoopRemoteSnapshot *remote, uint64_t now) {
    if (!enabled(a)) return a->status = COOP_ACTOR_DISABLED;
    CoopRemoteSnapshot s;
    int authorized = CoopRemoteSnapshotAt(remote, now, &s) && s.active && s.world_authorized;
    int changed = authorized && (s.session_id != a->session || s.connection_generation != a->connection ||
        s.world_generation != a->generation || s.epoch != a->epoch || s.player_id != a->player);
    if (a->handle && (a->retiring || !authorized || changed)) return retire(a);
    if (!authorized) return a->status = COOP_ACTOR_WAIT_WORLD;
    if (!a->handle) {
        if (now < a->retry_after) return a->status = COOP_ACTOR_CREATING;
        a->retry_after = now > UINT64_MAX - 500 ? UINT64_MAX : now + 500;
        uint32_t handle = a->ops.create(a->context, s.position);
        if (!handle)
            return a->status = COOP_ACTOR_BACKEND_REFUSED;
        a->handle = handle; a->session = s.session_id; a->connection = s.connection_generation;
        a->generation = s.world_generation; a->epoch = s.epoch; a->player = s.player_id;
        a->stopped = a->removal_started = a->retiring = 0;
        return a->status = COOP_ACTOR_CREATING; /* Do not move during creation. */
    }
    float p[3];
    if (!a->ops.position(a->context, a->handle, p) || !position_valid(p)) return retire(a);
    float dx = s.position[0] - p[0], dy = s.position[1] - p[1];
    float distance = sqrtf(dx*dx + dy*dy);
    /* No implicit teleport, vertical warp or unbounded catch-up through walls.
       These policy limits require real-game tuning; velocities use world
       coordinate units per second, not a native MoveDir mode byte. */
    if (distance > 20.0f || fabsf(s.position[2] - p[2]) > 3.0f) {
        if (!a->stopped) {
            if (!a->ops.stop(a->context, a->handle)) {
                a->retiring = 1; return a->status = COOP_ACTOR_BACKEND_REFUSED;
            }
            a->stopped = 1;
        }
        return a->status = COOP_ACTOR_TOO_FAR;
    }
    if (distance <= (a->stopped ? 0.20f : 0.10f)) {
        if (!a->stopped && !a->ops.stop(a->context, a->handle)) {
            a->retiring = 1; return a->status = COOP_ACTOR_BACKEND_REFUSED;
        }
        a->stopped = 1; return a->status = COOP_ACTOR_STANDING;
    }
    float speed = fminf(4.0f, distance * 4.0f);
    float velocity[2] = {dx / distance * speed, dy / distance * speed};
    if (!a->ops.move(a->context, a->handle, velocity)) return retire(a);
    a->stopped = 0; return a->status = COOP_ACTOR_MOVING;
}
