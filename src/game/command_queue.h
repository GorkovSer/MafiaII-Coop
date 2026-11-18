#ifndef COOP_COMMAND_QUEUE_H
#define COOP_COMMAND_QUEUE_H
#include <stddef.h>
#include <stdint.h>

typedef int (*CoopCommandRead)(void *context, uint32_t address, void *out, size_t size);
typedef enum {
    COOP_COMMAND_QUEUE_UNREADABLE = 0,
    COOP_COMMAND_QUEUE_CORRUPT,
    COOP_COMMAND_QUEUE_BAD_REFERENCE,
    COOP_COMMAND_QUEUE_CHANGED,
    COOP_COMMAND_QUEUE_VALID
} CoopCommandQueueStatus;
typedef struct {
    uint32_t command, vtable, references, flags;
    int32_t next;
    uint32_t queue; /* 0..4 occupied, 5 free */
} CoopCommandSlot;
typedef struct {
    CoopCommandSlot slots[8];
    int32_t heads[5], free_head;
    uint32_t free_count, used_count;
    /* Separately retained by the dispatcher; may legitimately be outside all queues. */
    uint32_t retained_command, retained_vtable, retained_references, retained_flags;
    uint32_t retained_is_queued;
} CoopCommandQueue;

/* Caller must resolve registry identity and enforce pinned image/game-thread
   context. Validity is a read-only layout observation, not permission to enqueue. */
CoopCommandQueueStatus CoopReadCommandQueue(CoopCommandRead read, void *context,
    uint32_t human, CoopCommandQueue *out);
#endif
