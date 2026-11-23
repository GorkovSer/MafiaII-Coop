#ifndef COOP_ENTITY_MAP_H
#define COOP_ENTITY_MAP_H
#include "entity_registry.h"
#include "../net/game_bridge.h"

#define COOP_ENTITY_MAP_CAPACITY 256u
typedef enum { COOP_NET_PLAYER=1, COOP_NET_NPC=2, COOP_NET_VEHICLE=3 } CoopNetEntityKind;
typedef struct {
    uint64_t network_id; /* Host-assigned; never reuse within a world/session. */
    uint32_t guid, kind, retiring;
} CoopEntityBinding;
typedef struct {
    CoopEntityBinding slots[COOP_ENTITY_MAP_CAPACITY];
    uint64_t session;
    uint32_t connection, generation, epoch, peer;
    unsigned int count, ready, quiescing, busy;
} CoopEntityMap;
typedef CoopEntityStatus (*CoopEntityAbsence)(void *context, uint32_t guid);
typedef struct { uint64_t session, next_id; } CoopEntityIdSource;
/* Host-only ID allocation. Preserve this source for the entire session;
   returning 0 on exhaustion is preferable to recycling an old ID. */
int CoopEntityIdSourceStart(CoopEntityIdSource *source, uint64_t session, unsigned int local_player);
uint64_t CoopEntityIdNext(CoopEntityIdSource *source, uint64_t current_session);

/* Game-thread bookkeeping only. Zero initialize before use; never reset live
   bindings. Native ownership remains with the engine backend, including when
   Bind refuses. This table does not authorize native calls or NPC authority.
   Spawn/despawn ordering and host authority must be validated before binding;
   network IDs must not be recycled in the same world/session. Incoming NPC
   and vehicle bindings are accepted only from player 1 (the session host). */
int CoopEntityMapBind(CoopEntityMap *map, const CoopRemoteSnapshot *current,
    uint64_t now_ms, uint64_t network_id, CoopNetEntityKind kind, uint32_t guid);
int CoopEntityMapFind(CoopEntityMap *map, const CoopRemoteSnapshot *current,
    uint64_t now_ms, uint64_t network_id, CoopEntityBinding *out);
/* Every update must synchronize CURRENT ReadRemoteWorld authority, even with
   no incoming entities. Withdrawal marks old bindings for retirement and a
   new world cannot bind until every old GUID has confirmed registry absence. */
int CoopEntityMapSynchronize(CoopEntityMap *map, const CoopRemoteSnapshot *current, uint64_t now_ms);
int CoopEntityMapRetire(CoopEntityMap *map, uint64_t network_id);
int CoopEntityMapFirstRetiring(const CoopEntityMap *map, CoopEntityBinding *out);
/* Call only after the backend's single native release. Callback must perform
   current pinned registry lookup; unreadable/mismatched is NOT absence. */
int CoopEntityMapConfirmRemoved(CoopEntityMap *map, uint64_t network_id,
    CoopEntityAbsence lookup, void *context);
#endif
