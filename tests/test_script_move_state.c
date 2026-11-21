/* Synthetic callback memory only; compile now, execute after user test hold. */
#include "../src/game/script_move_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    unsigned char s[0x28], u[12], c[0x8c];
    uint32_t count, link;
    unsigned int reads, change;
} Memory;
static void put(unsigned char *p, unsigned int at, uint32_t value) { memcpy(p+at, &value, 4); }
static int read_memory(void *context, uint32_t at, void *out, size_t size) {
    Memory *m = context;
    ++m->reads;
    if (m->change && m->reads == 6) put(m->c, 8, 2);
    const void *p = NULL; size_t available = 0;
    if (at == 0x1000) { p=m->s; available=sizeof(m->s); }
    if (at == 0x2000) { p=m->u; available=sizeof(m->u); }
    if (at == 0x3000) { p=m->c; available=sizeof(m->c); }
    if (at == 0x4000) { p=&m->count; available=4; }
    if (at == 0x5000) { p=&m->link; available=4; }
    if (!p || size > available) return 0;
    memcpy(out, p, size); return 1;
}
static void init(Memory *m) {
    memset(m, 0, sizeof(*m));
    put(m->s, 0, 0x190cc0c); put(m->s, 4, 2); put(m->s, 8, 3); put(m->s, 0x14, 0x2000);
    put(m->u, 0, 0x197bd94); put(m->u, 4, 0x3000); put(m->u, 8, 0x4000);
    put(m->c, 0, 0x197bccc); put(m->c, 4, 0x5000);
    m->c[0xd]=1; m->count=2; m->link=0x1000;
}
static void refused(Memory *m, CoopScriptStateStatus expected) {
    CoopScriptState out; memset(&out, 0xa5, sizeof(out));
    assert(CoopReadScriptMoveState(read_memory, m, 0x1000, &out) == expected);
    assert(!out.sync_flags && !out.command && !out.command_state &&
        !out.linked_to_sync && !out.command_flag_d);
}
int main(void) {
    Memory m; CoopScriptState out;
    init(&m);
    assert(CoopReadScriptMoveState(read_memory, &m, 0x1000, &out) == COOP_SCRIPT_STATE_VALID);
    assert(out.command == 0x3000 && out.sync_flags == 3 && out.linked_to_sync && out.command_flag_d == 1);
    for (uint32_t state=1; state<=3; ++state) {
        init(&m); put(m.c, 8, state); put(m.c, 4, 0); put(m.s, 8, 2);
        assert(CoopReadScriptMoveState(read_memory, &m, 0x1000, &out) == COOP_SCRIPT_STATE_VALID);
        assert(out.command_state == state && !out.linked_to_sync);
    }
    init(&m); m.count=0; refused(&m, COOP_SCRIPT_STATE_MISMATCH);
    init(&m); m.link=0x9999; refused(&m, COOP_SCRIPT_STATE_MISMATCH);
    init(&m); put(m.c, 0, 0x19661bc); refused(&m, COOP_SCRIPT_STATE_MISMATCH);
    init(&m); put(m.c, 8, 4); refused(&m, COOP_SCRIPT_STATE_MISMATCH);
    init(&m); put(m.u, 8, 0xfffffffeu); refused(&m, COOP_SCRIPT_STATE_UNREADABLE);
    init(&m); m.change=1; refused(&m, COOP_SCRIPT_STATE_CHANGED);
    init(&m); put(m.s, 8, 1); refused(&m, COOP_SCRIPT_STATE_MISMATCH);
    assert(CoopReadScriptMoveState(NULL, NULL, 0, &out) == COOP_SCRIPT_STATE_UNREADABLE);
    puts("Script movement state reader fixture passed");
    return 0;
}
