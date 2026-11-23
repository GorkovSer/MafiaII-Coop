#include "entity_map.h"
#include <string.h>

int CoopEntityIdSourceStart(CoopEntityIdSource *s, uint64_t session, unsigned int player) {
    if (!s || !session || player!=1) return 0;
    if (s->session==session) return s->next_id!=0; /* Never rewind this session. */
    s->session=session; s->next_id=1; return 1;
}
uint64_t CoopEntityIdNext(CoopEntityIdSource *s, uint64_t session) {
    if (!s || !session || s->session!=session || !s->next_id) return 0;
    uint64_t id=s->next_id;
    s->next_id=id==UINT64_MAX ? 0 : id+1;
    return id;
}

static void retire_all(CoopEntityMap *m) {
    for (unsigned int i=0; i<COOP_ENTITY_MAP_CAPACITY; ++i)
        if (m->slots[i].network_id) m->slots[i].retiring=1;
}
int CoopEntityMapSynchronize(CoopEntityMap *m, const CoopRemoteSnapshot *current, uint64_t now) {
    if (!m || m->busy) return 0;
    CoopRemoteSnapshot s;
    int valid=CoopRemoteSnapshotAt(current, now, &s) && s.active && s.world_authorized;
    int same=valid && m->session==s.session_id && m->connection==s.connection_generation &&
        m->generation==s.world_generation && m->epoch==s.epoch && m->peer==s.player_id;
    if (!valid || !same || m->quiescing) {
        m->ready=0; retire_all(m);
        if (m->count) { m->quiescing=1; return 0; }
        m->quiescing=0;
        if (!valid) return 0;
        m->session=s.session_id; m->connection=s.connection_generation;
        m->generation=s.world_generation; m->epoch=s.epoch; m->peer=s.player_id;
    }
    m->ready=1; return 1;
}
int CoopEntityMapBind(CoopEntityMap *m, const CoopRemoteSnapshot *current,
    uint64_t now, uint64_t id, CoopNetEntityKind kind, uint32_t guid) {
    if (!id || !guid || kind<COOP_NET_PLAYER || kind>COOP_NET_VEHICLE ||
        !CoopEntityMapSynchronize(m, current, now)) return 0;
    if (kind!=COOP_NET_PLAYER && m->peer!=1) return 0;
    unsigned int free_slot=COOP_ENTITY_MAP_CAPACITY;
    for (unsigned int i=0; i<COOP_ENTITY_MAP_CAPACITY; ++i) {
        const CoopEntityBinding *b=&m->slots[i];
        if (b->network_id==id)
            return !b->retiring && b->guid==guid && b->kind==(uint32_t)kind;
        if (b->network_id && b->guid==guid) return 0;
        if (!b->network_id && free_slot==COOP_ENTITY_MAP_CAPACITY) free_slot=i;
    }
    if (free_slot==COOP_ENTITY_MAP_CAPACITY) return 0;
    m->slots[free_slot]=(CoopEntityBinding){id,guid,(uint32_t)kind,0};
    ++m->count; return 1;
}
int CoopEntityMapFind(CoopEntityMap *m, const CoopRemoteSnapshot *current,
    uint64_t now, uint64_t id, CoopEntityBinding *out) {
    if (!out) return 0;
    memset(out,0,sizeof(*out));
    if (!id || !CoopEntityMapSynchronize(m,current,now)) return 0;
    for (unsigned int i=0; i<COOP_ENTITY_MAP_CAPACITY; ++i)
        if (m->slots[i].network_id==id && !m->slots[i].retiring) {
            *out=m->slots[i]; return 1;
        }
    return 0;
}
int CoopEntityMapRetire(CoopEntityMap *m, uint64_t id) {
    if (!m || m->busy || !id) return 0;
    for (unsigned int i=0; i<COOP_ENTITY_MAP_CAPACITY; ++i)
        if (m->slots[i].network_id==id) { m->slots[i].retiring=1; return 1; }
    return 0;
}
int CoopEntityMapFirstRetiring(const CoopEntityMap *m, CoopEntityBinding *out) {
    if (!out) return 0;
    memset(out,0,sizeof(*out));
    if (!m || m->busy) return 0;
    for (unsigned int i=0; i<COOP_ENTITY_MAP_CAPACITY; ++i)
        if (m->slots[i].network_id && m->slots[i].retiring) { *out=m->slots[i]; return 1; }
    return 0;
}
int CoopEntityMapConfirmRemoved(CoopEntityMap *m, uint64_t id,
    CoopEntityAbsence lookup, void *context) {
    if (!m || m->busy || !id || !lookup) return 0;
    for (unsigned int i=0; i<COOP_ENTITY_MAP_CAPACITY; ++i) {
        CoopEntityBinding *b=&m->slots[i];
        if (b->network_id!=id || !b->retiring) continue;
        m->busy=1;
        CoopEntityStatus status=lookup(context,b->guid);
        if (status==COOP_ENTITY_ABSENT) { memset(b,0,sizeof(*b)); --m->count; }
        m->busy=0;
        return status==COOP_ENTITY_ABSENT;
    }
    return 0;
}
