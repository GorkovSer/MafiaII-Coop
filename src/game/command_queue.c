/* Pinned Human queue layout traced from 0x916e30 / 0x94d320 / 0x94d390.
   No game calls, command allocations, reference changes or queue writes. */
#include "command_queue.h"
#include <limits.h>
#include <string.h>

static int take(CoopCommandRead read, void *context, uint64_t at, void *out, size_t size) {
    return at && at <= UINT32_MAX && size <= UINT64_C(0x100000000) - at &&
        read(context, (uint32_t)at, out, size);
}
static uint32_t word(const unsigned char *p) {
    uint32_t value; memcpy(&value, p, 4); return value;
}
static int32_t index_at(const unsigned char *p) {
    int32_t value; memcpy(&value, p, 4); return value;
}
static int index_valid(int32_t index) { return index == -1 || (index >= 0 && index < 8); }

CoopCommandQueueStatus CoopReadCommandQueue(CoopCommandRead read, void *context,
    uint32_t human, CoopCommandQueue *out) {
    if (!out) return COOP_COMMAND_QUEUE_UNREADABLE;
    memset(out, 0, sizeof(*out));
    if (!read || !human) return COOP_COMMAND_QUEUE_UNREADABLE;
    unsigned char raw[0x58], again[0x58];
    if (!take(read, context, (uint64_t)human + 0x138, raw, sizeof(raw)))
        return COOP_COMMAND_QUEUE_UNREADABLE;
    CoopCommandQueue candidate = {0};
    for (unsigned int i = 0; i < 8; ++i) {
        candidate.slots[i].command = word(raw + i * 8);
        candidate.slots[i].next = index_at(raw + i * 8 + 4);
        if (!index_valid(candidate.slots[i].next)) return COOP_COMMAND_QUEUE_CORRUPT;
    }
    for (unsigned int i = 0; i < 5; ++i) {
        candidate.heads[i] = index_at(raw + 0x40 + i * 4);
        if (!index_valid(candidate.heads[i])) return COOP_COMMAND_QUEUE_CORRUPT;
    }
    candidate.free_head = index_at(raw + 0x54);
    if (!index_valid(candidate.free_head)) return COOP_COMMAND_QUEUE_CORRUPT;
    unsigned int seen = 0;
    /* Fixed eight slots: a repeated index catches cycles and cross-list aliasing. */
    for (unsigned int queue = 0; queue < 6; ++queue) {
        int32_t index = queue == 5 ? candidate.free_head : candidate.heads[queue];
        while (index != -1) {
            if (!index_valid(index) || (seen & (1u << (unsigned int)index)))
                return COOP_COMMAND_QUEUE_CORRUPT;
            seen |= 1u << (unsigned int)index;
            CoopCommandSlot *slot = &candidate.slots[index];
            slot->queue = queue;
            if (queue == 5) {
                if (slot->command) return COOP_COMMAND_QUEUE_CORRUPT;
                ++candidate.free_count;
            } else {
                if (!slot->command || slot->command % 4) return COOP_COMMAND_QUEUE_BAD_REFERENCE;
                uint32_t header[4];
                if (!take(read, context, slot->command, header, sizeof(header)))
                    return COOP_COMMAND_QUEUE_UNREADABLE;
                slot->vtable = header[0]; slot->references = header[1]; slot->flags = header[3];
                if (!slot->vtable || slot->vtable % 4 || !slot->references ||
                    slot->references > INT32_MAX || !(slot->flags & 0x400u))
                    return COOP_COMMAND_QUEUE_BAD_REFERENCE;
                ++candidate.used_count;
            }
            index = slot->next;
        }
    }
    if (seen != 0xffu || candidate.free_count + candidate.used_count != 8)
        return COOP_COMMAND_QUEUE_CORRUPT;
    if (!take(read, context, (uint64_t)human + 0x1a0, &candidate.retained_command, 4))
        return COOP_COMMAND_QUEUE_UNREADABLE;
    if (candidate.retained_command) {
        uint32_t header[4];
        if (candidate.retained_command % 4) return COOP_COMMAND_QUEUE_BAD_REFERENCE;
        if (!take(read, context, candidate.retained_command, header, sizeof(header)))
            return COOP_COMMAND_QUEUE_UNREADABLE;
        candidate.retained_vtable = header[0]; candidate.retained_references = header[1];
        candidate.retained_flags = header[3];
        if (!header[0] || header[0] % 4 || !header[1] || header[1] > INT32_MAX)
            return COOP_COMMAND_QUEUE_BAD_REFERENCE;
        /* Do not require flag 0x400: a retained command may already be dequeued. */
        for (unsigned int i = 0; i < 8; ++i)
            if (candidate.slots[i].queue < 5 && candidate.slots[i].command == candidate.retained_command)
                candidate.retained_is_queued = 1;
    }
    if (!take(read, context, (uint64_t)human + 0x138, again, sizeof(again)))
        return COOP_COMMAND_QUEUE_UNREADABLE;
    if (memcmp(raw, again, sizeof(raw))) return COOP_COMMAND_QUEUE_CHANGED;
    uint32_t retained_again;
    if (!take(read, context, (uint64_t)human + 0x1a0, &retained_again, 4))
        return COOP_COMMAND_QUEUE_UNREADABLE;
    if (retained_again != candidate.retained_command) return COOP_COMMAND_QUEUE_CHANGED;
    *out = candidate;
    return COOP_COMMAND_QUEUE_VALID;
}
