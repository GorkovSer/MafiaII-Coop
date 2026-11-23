#ifndef COOP_PLAYER_CONTROL_H
#define COOP_PLAYER_CONTROL_H
#include <stddef.h>
#include <stdint.h>

typedef int (*CoopPlayerControlRead)(void *, uint32_t, void *, size_t);
typedef enum {
    COOP_PLAYER_CONTROL_UNREADABLE, COOP_PLAYER_CONTROL_LAYOUT,
    COOP_PLAYER_CONTROL_CHANGED, COOP_PLAYER_CONTROL_VALID
} CoopPlayerControlStatus;
typedef struct {
    uint32_t mode;
    int native_lock_query;
} CoopPlayerControl;
/* Read-only equivalent of the pinned Player lock query, NOT a movement lease.
   Caller establishes full image hash, game thread and current entity identity.
   No retained references, engine calls or permission to lock the local player. */
CoopPlayerControlStatus CoopPlayerControlObserve(CoopPlayerControlRead read,
    void *context, uint32_t player, uint32_t guid, CoopPlayerControl *out);
#endif
