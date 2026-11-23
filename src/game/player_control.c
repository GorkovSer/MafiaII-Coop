#include "player_control.h"
#include <string.h>

typedef struct { uint32_t table, guid, query, setter, mode; } Snapshot;
static int take(CoopPlayerControlRead read, void *context, uint32_t base,
    uint32_t offset, uint32_t *out) {
    if (!base || offset > UINT32_MAX-base || base+offset > UINT32_MAX-3)
        return 0;
    return read(context, base+offset, out, 4);
}
static int snapshot(CoopPlayerControlRead read, void *context,
    uint32_t player, Snapshot *out) {
    return take(read, context, player, 0, &out->table) &&
        take(read, context, player, 0x1c, &out->guid) &&
        take(read, context, out->table, 0x14c, &out->query) &&
        take(read, context, out->table, 0x168, &out->setter) &&
        take(read, context, player, 0x310, &out->mode);
}
CoopPlayerControlStatus CoopPlayerControlObserve(CoopPlayerControlRead read,
    void *context, uint32_t player, uint32_t guid, CoopPlayerControl *out) {
    if (!out) return COOP_PLAYER_CONTROL_UNREADABLE;
    memset(out, 0, sizeof(*out));
    if (!read || !player || !guid || (guid & 255) != 0x10)
        return COOP_PLAYER_CONTROL_LAYOUT;
    Snapshot first = {0}, second = {0};
    if (!snapshot(read, context, player, &first)) return COOP_PLAYER_CONTROL_UNREADABLE;
    if (first.table != 0x18f14b8 || first.guid != guid ||
        first.query != 0x41fcd0 || first.setter != 0x438230)
        return COOP_PLAYER_CONTROL_LAYOUT;
    if (!snapshot(read, context, player, &second)) return COOP_PLAYER_CONTROL_UNREADABLE;
    if (first.table != second.table || first.guid != second.guid ||
        first.query != second.query || first.setter != second.setter ||
        first.mode != second.mode) return COOP_PLAYER_CONTROL_CHANGED;
    out->mode = first.mode;
    out->native_lock_query = first.mode == 10;
    return COOP_PLAYER_CONTROL_VALID;
}
