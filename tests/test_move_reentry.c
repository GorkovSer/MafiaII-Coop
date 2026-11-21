/* Refusing guards only: no Mafia process or native engine binding is used.
   This fixture is compiled but must not run while the user test hold applies. */
#include "../src/game/move_command.h"
#include <assert.h>
#include <stdio.h>
typedef struct { CoopMoveCommand *owner; unsigned int calls; } Context;
static int guard(void *context, uint32_t guid, uint32_t human, unsigned int op) {
    Context *c = context;
    CoopMoveCommand *o = c->owner;
    assert(++c->calls == 1); /* Recursive callbacks must never reach here. */
    assert(guid == 123 && human == 456);
    assert(op >= COOP_MOVE_EDIT && op <= COOP_MOVE_RELEASE);
    assert(o->busy == 1);
    CoopMoveCommand before = *o;
    const float v[2] = {1, 0};
    assert(CoopMoveCommandCreate(o, guard, context, 789, 999) == COOP_MOVE_REFUSED);
    assert(CoopMoveCommandInput(o, v, v, 1, 2) == COOP_MOVE_REFUSED);
    assert(CoopMoveCommandCancel(o) == COOP_MOVE_REFUSED);
    assert(CoopMoveCommandRelease(o) == COOP_MOVE_REFUSED);
    assert(o->human == before.human && o->guid == before.guid &&
        o->command == before.command && o->guard == before.guard &&
        o->context == before.context && o->busy == before.busy);
    return 0; /* Stops outer operation before ABI or process-memory checks. */
}
int main(void) {
    CoopMoveCommand o = {0};
    Context c = {&o, 0};
    const float v[2] = {1, 0};
    assert(CoopMoveCommandCreate(&o, guard, &c, 123, 456) == COOP_MOVE_REFUSED);
    assert(c.calls == 1 && !o.busy && !o.command);
    c.calls = 0;
    assert(CoopMoveCommandInput(&o, v, v, 1, 2) == COOP_MOVE_REFUSED);
    assert(c.calls == 1 && !o.busy && !o.command);
    c.calls = 0;
    assert(CoopMoveCommandCancel(&o) == COOP_MOVE_REFUSED);
    assert(c.calls == 1 && !o.busy);
    c.calls = 0;
    assert(CoopMoveCommandRelease(&o) == COOP_MOVE_REFUSED);
    assert(c.calls == 1 && !o.busy);
    assert(CoopMoveCommandCreate(NULL, guard, &c, 123, 456) == COOP_MOVE_REFUSED);
    assert(CoopMoveCommandInput(NULL, v, v, 1, 2) == COOP_MOVE_REFUSED);
    assert(CoopMoveCommandCancel(NULL) == COOP_MOVE_REFUSED);
    assert(CoopMoveCommandRelease(NULL) == COOP_MOVE_REFUSED);
    puts("Move owner recursive entry refusal passed");
    return 0;
}
