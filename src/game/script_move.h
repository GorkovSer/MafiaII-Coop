#ifndef COOP_SCRIPT_MOVE_H
#define COOP_SCRIPT_MOVE_H
#include <stdint.h>
#include "script_move_state.h"
enum { COOP_SCRIPT_MOVE_START = 1, COOP_SCRIPT_MOVE_RELEASE = 2,
    COOP_SCRIPT_MOVE_OBSERVE = 3, COOP_SCRIPT_MOVE_END = 4 };
typedef int (*CoopScriptMoveGuard)(void *context, uint32_t guid, uint32_t human,
    unsigned int operation);
typedef struct {
    uint32_t human, guid, sync; /* Exactly one owned intrusive Sync reference. */
    CoopScriptMoveGuard guard;
    void *context;
    unsigned int busy;
} CoopScriptMove;
typedef enum {
    COOP_SCRIPT_MOVE_REFUSED, COOP_SCRIPT_MOVE_HANDLE_CREATED,
    COOP_SCRIPT_MOVE_INCOMPLETE, COOP_SCRIPT_MOVE_REFERENCE_RELEASED,
    COOP_SCRIPT_MOVE_END_REQUESTED, COOP_SCRIPT_MOVE_TERMINAL_OBSERVED
} CoopScriptMoveResult;
/* Candidate, not attached to callbacks. Zero-initialize once; never reset a
   live owner. Calls must be serialized on the verified game thread.
   Guard always checks full EXE hash/base/thread; START additionally checks a
   current mod-owned registry identity and fresh agreed world. Never use story
   NPCs. The native AI scheduler handles this command, rather than bypassing it.
   Scheduler arbitration and the native OOM/exception contract remain untested.
   A created Sync handle is NOT proof of acceptance, physical motion or success. */
CoopScriptMoveResult CoopScriptMoveStart(CoopScriptMove *owner,
    CoopScriptMoveGuard guard, void *context, uint32_t guid, uint32_t human,
    const float position[3], unsigned int mode, const float end_direction[3]);
/* Drops only our reference. Does not cancel movement, notify Done or prove
   physical stop. Guard must permit releasing this owned ref after world/actor
   withdrawal without dereferencing a dead Human. */
CoopScriptMoveResult CoopScriptMoveRelease(CoopScriptMove *owner);
/* Observation guard needs pinned image/thread and our owned Sync ref, and must
   not require a live Human or world. No native mutation, stop or success verdict. */
CoopScriptStateStatus CoopScriptMoveObserve(CoopScriptMove *owner, CoopScriptState *out);
/* Candidate request through native command abort-flag setter. END guard
   verifies image/thread and our owned Sync/command, even after world withdrawal.
   Only linked raw-state-0 commands receive the request; command state itself
   stays untouched for native task result/Sync propagation. 1..3 are left intact. Neither
   END_REQUESTED nor TERMINAL_OBSERVED establishes stopped motion/task removal.
   Caller must preserve the Sync ref and await separately verified stop. */
CoopScriptMoveResult CoopScriptMoveRequestEnd(CoopScriptMove *owner);
#endif
