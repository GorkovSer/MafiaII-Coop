/* Deferred while user plays Cyberpunk: fixture source, not run on authoring. */
#include "../src/game/command_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char memory[0x10000];
static const uint32_t human = 0x1000, command = 0x3000;
static int missing_header, change_second_read;
static unsigned int reads, queue_reads;
static void u32(uint32_t at, uint32_t value) { memcpy(memory + at, &value, 4); }
static void next(unsigned int slot, int32_t value) { u32(human + 0x13c + slot * 8, (uint32_t)value); }
static void head(unsigned int queue, int32_t value) { u32(human + 0x178 + queue * 4, (uint32_t)value); }
static void free_head(int32_t value) { u32(human + 0x18c, (uint32_t)value); }
static void reference(unsigned int slot) {
    uint32_t address = command + slot * 0x100;
    u32(human + 0x138 + slot * 8, address);
    u32(address, 0x18f14b0); u32(address + 4, 1); u32(address + 0x0c, 0x401);
}
static int read_memory(void *context, uint32_t at, void *out, size_t size) {
    (void)context; ++reads;
    if (at >= sizeof(memory) || size > sizeof(memory) - at || (missing_header && at == command)) return 0;
    memcpy(out, memory + at, size);
    if (at == human + 0x138) {
        ++queue_reads;
        if (change_second_read && queue_reads == 2) ((unsigned char *)out)[0x54] ^= 1;
    }
    return 1;
}
static void empty(void) {
    memset(memory, 0, sizeof(memory)); missing_header = change_second_read = 0;
    for (unsigned int i = 0; i < 8; ++i) next(i, i == 7 ? -1 : (int32_t)i + 1);
    for (unsigned int i = 0; i < 5; ++i) head(i, -1);
    free_head(0);
}
static void single(void) { empty(); reference(0); next(0, -1); head(0, 0); free_head(1); }
static void check(uint32_t address, CoopCommandQueueStatus expected, unsigned int used, unsigned int free_slots) {
    CoopCommandQueue result, zero = {0}; memset(&result, 0xaa, sizeof(result));
    reads = queue_reads = 0;
    CoopCommandQueueStatus actual = CoopReadCommandQueue(read_memory, NULL, address, &result);
    if (actual != expected || reads > 10 ||
        (actual == COOP_COMMAND_QUEUE_VALID ? result.used_count != used || result.free_count != free_slots :
         memcmp(&result, &zero, sizeof(result)) != 0)) {
        fprintf(stderr, "queue status=%d expected=%d used=%u free=%u reads=%u\n",
            actual, expected, result.used_count, result.free_count, reads); exit(1);
    }
}
int main(void) {
    empty(); check(human, COOP_COMMAND_QUEUE_VALID, 0, 8);
    single(); check(human, COOP_COMMAND_QUEUE_VALID, 1, 7);
    head(1, 0); check(human, COOP_COMMAND_QUEUE_CORRUPT, 0, 0); /* shared slot */
    single(); free_head(0); check(human, COOP_COMMAND_QUEUE_CORRUPT, 0, 0);
    single(); next(0, 0); check(human, COOP_COMMAND_QUEUE_CORRUPT, 0, 0);
    single(); next(0, 8); check(human, COOP_COMMAND_QUEUE_CORRUPT, 0, 0);
    single(); next(0, -2); check(human, COOP_COMMAND_QUEUE_CORRUPT, 0, 0);
    single(); free_head(2); check(human, COOP_COMMAND_QUEUE_CORRUPT, 0, 0); /* orphan */
    empty(); reference(0); check(human, COOP_COMMAND_QUEUE_CORRUPT, 0, 0); /* free owns reference */
    single(); u32(command + 4, 0); check(human, COOP_COMMAND_QUEUE_BAD_REFERENCE, 0, 0);
    single(); u32(command + 4, 0x80000000); check(human, COOP_COMMAND_QUEUE_BAD_REFERENCE, 0, 0);
    single(); u32(command + 0x0c, 1); check(human, COOP_COMMAND_QUEUE_BAD_REFERENCE, 0, 0);
    single(); u32(human + 0x138, command + 1); check(human, COOP_COMMAND_QUEUE_BAD_REFERENCE, 0, 0);
    single(); missing_header = 1; check(human, COOP_COMMAND_QUEUE_UNREADABLE, 0, 0);
    single(); change_second_read = 1; check(human, COOP_COMMAND_QUEUE_CHANGED, 0, 0);
    empty(); head(0, 0); free_head(-1);
    for (unsigned int i = 0; i < 8; ++i) reference(i);
    check(human, COOP_COMMAND_QUEUE_VALID, 8, 0); /* full is valid, no enqueue capacity */
    check(0xffffff00, COOP_COMMAND_QUEUE_UNREADABLE, 0, 0);
    check(0, COOP_COMMAND_QUEUE_UNREADABLE, 0, 0);
    puts("PASS production command queue: empty/full, topology, references, bounds and changed snapshot");
    return 0;
}
