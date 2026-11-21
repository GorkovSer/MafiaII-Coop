#include "script_move_state.h"
#include <string.h>
static int take(CoopScriptStateRead read, void *context, uint32_t base,
    void *out, size_t size) {
    return base && size <= UINT64_C(0x100000000)-base && read(context, base, out, size);
}
static uint32_t word(const unsigned char *p) {
    uint32_t v; memcpy(&v, p, 4); return v;
}
static CoopScriptStateStatus snapshot(CoopScriptStateRead read, void *context,
    uint32_t sync, CoopScriptState *out, uint32_t identity[3]) {
    unsigned char s[0x18], u[12], c[0x10];
    uint32_t count;
    if (!take(read, context, sync, s, sizeof(s))) return COOP_SCRIPT_STATE_UNREADABLE;
    if (word(s) != 0x190cc0c || !word(s+4) || word(s+4) >= 0x7fffffffu ||
        !(s[8] & 2) || !word(s+0x14)) return COOP_SCRIPT_STATE_MISMATCH;
    identity[0] = word(s+0x14);
    if (!take(read, context, identity[0], u, sizeof(u))) return COOP_SCRIPT_STATE_UNREADABLE;
    if (word(u) != 0x197bd94 || !word(u+4) || !word(u+8)) return COOP_SCRIPT_STATE_MISMATCH;
    identity[1] = word(u+4); identity[2] = word(u+8);
    if (!take(read, context, identity[2], &count, 4) ||
        !take(read, context, identity[1], c, sizeof(c))) return COOP_SCRIPT_STATE_UNREADABLE;
    if (!count || count >= 0x7fffffffu || word(c) != 0x197bccc || word(c+8) > 3)
        return COOP_SCRIPT_STATE_MISMATCH;
    out->sync_flags = s[8]; out->command = identity[1];
    out->command_state = word(c+8); out->command_flag_d = c[0xd];
    if (word(c+4)) {
        uint32_t linked;
        if (!take(read, context, word(c+4), &linked, 4)) return COOP_SCRIPT_STATE_UNREADABLE;
        if (linked != sync) return COOP_SCRIPT_STATE_MISMATCH;
        out->linked_to_sync = 1;
    }
    return COOP_SCRIPT_STATE_VALID;
}
CoopScriptStateStatus CoopReadScriptMoveState(CoopScriptStateRead read, void *context,
    uint32_t sync, CoopScriptState *out) {
    if (!out) return COOP_SCRIPT_STATE_UNREADABLE;
    memset(out, 0, sizeof(*out));
    if (!read || !sync) return COOP_SCRIPT_STATE_UNREADABLE;
    CoopScriptState a = {0}, b = {0}; uint32_t ia[3], ib[3];
    CoopScriptStateStatus status = snapshot(read, context, sync, &a, ia);
    if (status != COOP_SCRIPT_STATE_VALID) return status;
    status = snapshot(read, context, sync, &b, ib);
    if (status != COOP_SCRIPT_STATE_VALID) return status;
    if (memcmp(ia, ib, sizeof(ia)) || a.sync_flags != b.sync_flags ||
        a.command != b.command || a.command_state != b.command_state ||
        a.linked_to_sync != b.linked_to_sync || a.command_flag_d != b.command_flag_d)
        return COOP_SCRIPT_STATE_CHANGED;
    *out = b; return COOP_SCRIPT_STATE_VALID;
}
