#ifndef COOP_SCRIPT_MOVE_STATE_H
#define COOP_SCRIPT_MOVE_STATE_H
#include <stddef.h>
#include <stdint.h>
typedef int (*CoopScriptStateRead)(void *context, uint32_t address, void *out, size_t size);
typedef enum {
    COOP_SCRIPT_STATE_UNREADABLE, COOP_SCRIPT_STATE_MISMATCH,
    COOP_SCRIPT_STATE_CHANGED, COOP_SCRIPT_STATE_VALID
} CoopScriptStateStatus;
typedef struct {
    uint32_t sync_flags, command, command_state;
    uint32_t linked_to_sync;
    uint32_t command_references; /* Observed native count, not ownership acquired. */
    uint8_t command_flag_d; /* Raw serialized byte, not a cancellation verdict. */
} CoopScriptState;
/* Owned Sync reference and pinned image/game-thread context required externally.
   This acquires no refs and makes no engine calls. VALID is layout/state only,
   never proof of acceptance, success, cancellation, deletion or physical stop. */
CoopScriptStateStatus CoopReadScriptMoveState(CoopScriptStateRead read, void *context,
    uint32_t sync, CoopScriptState *out);
#endif
