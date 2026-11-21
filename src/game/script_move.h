#ifndef COOP_SCRIPT_MOVE_H
#define COOP_SCRIPT_MOVE_H
#include <stdint.h>
enum { COOP_SCRIPT_MOVE_START = 1, COOP_SCRIPT_MOVE_RELEASE = 2 };
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
    COOP_SCRIPT_MOVE_INCOMPLETE, COOP_SCRIPT_MOVE_REFERENCE_RELEASED
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
   withdrawal without dereferencing a dead Human. Cancellation is not bound yet. */
CoopScriptMoveResult CoopScriptMoveRelease(CoopScriptMove *owner);
#endif
