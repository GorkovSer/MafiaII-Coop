/* Production owner operations with refusing guards; no game/native calls. */
#include "../src/game/script_move.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct { CoopScriptMove *owner; unsigned int operation, calls; } Context;
static int guard(void *context, uint32_t guid, uint32_t human, unsigned int op) {
    Context *c = context;
    CoopScriptMove *o = c->owner, before = *o;
    const float p[3] = {1, 2, 3};
    CoopScriptState state;
    assert(++c->calls == 1 && op == c->operation);
    assert(guid == 123 && human == 456 && o->busy);
    assert(CoopScriptMoveStart(o, guard, c, 999, 888, p, 0, p) == COOP_SCRIPT_MOVE_REFUSED);
    assert(CoopScriptMoveRelease(o) == COOP_SCRIPT_MOVE_REFUSED);
    assert(CoopScriptMoveRequestEnd(o) == COOP_SCRIPT_MOVE_REFUSED);
    memset(&state, 0xa5, sizeof(state));
    assert(CoopScriptMoveObserve(o, &state) == COOP_SCRIPT_STATE_UNREADABLE);
    assert(!state.sync_flags && !state.command && !state.command_state &&
        !state.linked_to_sync && !state.command_flag_d);
    assert(o->human == before.human && o->guid == before.guid &&
        o->sync == before.sync && o->guard == before.guard &&
        o->context == before.context && o->busy == before.busy);
    return 0; /* Prevent even ABI reads and preserve the owned reference. */
}
int main(void) {
    CoopScriptMove o = {0};
    Context c = {&o, COOP_SCRIPT_MOVE_START, 0};
    const float p[3] = {1, 2, 3};
    CoopScriptState state;
    assert(CoopScriptMoveStart(&o, guard, &c, 123, 456, p, 0, p) == COOP_SCRIPT_MOVE_REFUSED);
    assert(c.calls == 1 && !o.busy && !o.sync);
    /* Owned reference is an opaque test token, never mapped/dereferenced. */
    o.sync = 0x1000;
    c.calls = 0; c.operation = COOP_SCRIPT_MOVE_END;
    assert(CoopScriptMoveRequestEnd(&o) == COOP_SCRIPT_MOVE_REFUSED);
    assert(c.calls == 1 && !o.busy && o.sync == 0x1000);
    c.calls = 0; c.operation = COOP_SCRIPT_MOVE_OBSERVE;
    assert(CoopScriptMoveObserve(&o, &state) == COOP_SCRIPT_STATE_UNREADABLE);
    assert(c.calls == 1 && !o.busy && o.sync == 0x1000);
    c.calls = 0; c.operation = COOP_SCRIPT_MOVE_RELEASE;
    assert(CoopScriptMoveRelease(&o) == COOP_SCRIPT_MOVE_REFUSED);
    assert(c.calls == 1 && !o.busy && o.sync == 0x1000);
    assert(CoopScriptMoveRequestEnd(NULL) == COOP_SCRIPT_MOVE_REFUSED);
    puts("Script movement owner recursive entry and reference preservation passed");
    return 0;
}
