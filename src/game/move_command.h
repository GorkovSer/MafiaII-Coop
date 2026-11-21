#ifndef COOP_MOVE_COMMAND_H
#define COOP_MOVE_COMMAND_H
#include <stdint.h>

/* Candidate native backend component, never enabled by observer callbacks.
   Guard must establish full pinned image, game thread and owned actor identity.
   EDIT additionally requires fresh world authority and exclusive AI control;
   CLEANUP must remain possible after world authority is withdrawn. */
enum { COOP_MOVE_EDIT = 1, COOP_MOVE_CLEANUP = 2, COOP_MOVE_RELEASE = 3 };
typedef int (*CoopMoveGuard)(void *context, uint32_t guid, uint32_t human, unsigned int operation);
typedef struct {
    uint32_t human, guid, command; /* exactly one owned Command reference */
    CoopMoveGuard guard;
    void *context;
} CoopMoveCommand;
typedef enum {
    COOP_MOVE_REFUSED, COOP_MOVE_CREATED, COOP_MOVE_UPDATED,
    COOP_MOVE_CANCEL_REQUESTED, COOP_MOVE_RELEASED, COOP_MOVE_INCOMPLETE
} CoopMoveResult;
/* Start with a zero-initialized owner. INCOMPLETE may still leave an owned
   command; caller must preserve it for cleanup, never reset/overwrite it. */
CoopMoveResult CoopMoveCommandCreate(CoopMoveCommand *owner, CoopMoveGuard guard,
    void *context, uint32_t guid, uint32_t human);
CoopMoveResult CoopMoveCommandInput(CoopMoveCommand *owner,
    const float facing[2], const float motion[2], float multiplier, uint8_t mode);
CoopMoveResult CoopMoveCommandCancel(CoopMoveCommand *owner);
/* Release only caller's reference; does NOT prove stopped/unqueued/destroyed. */
CoopMoveResult CoopMoveCommandRelease(CoopMoveCommand *owner);
#endif
