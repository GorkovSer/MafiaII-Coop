#include "../src/game/entity_registry.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static unsigned char memory[0x10000], valid[0x10000];
static unsigned int reads;
static void put(uint32_t at, const void *value, size_t size) {
    memcpy(memory + at, value, size); memset(valid + at, 1, size);
}
static void u32(uint32_t at, uint32_t value) { put(at, &value, 4); }
static void i16(uint32_t at, int16_t value) { put(at, &value, 2); }
static int read_memory(void *context, uint32_t at, void *out, size_t size) {
    (void)context; ++reads;
    if (at >= sizeof(memory) || size > sizeof(memory) - at) return 0;
    for (size_t i = 0; i < size; ++i) if (!valid[at + i]) return 0;
    memcpy(out, memory + at, size); return 1;
}
static void check(uint32_t manager, uint32_t guid, CoopEntityStatus expected, uint32_t address) {
    uint32_t result = 0xdeadbeef;
    reads = 0;
    CoopEntityStatus actual = CoopEntityLookup(read_memory, NULL, manager, guid, &result);
    if (actual != expected || result != address || reads > 10110) {
        fprintf(stderr, "registry result=%d expected=%d pointer=%x reads=%u\n", actual, expected, result, reads);
        exit(1);
    }
}
int main(void) {
    const uint32_t manager = 0x1000, base = 0x1004, buckets = 0x2000;
    const uint32_t entries = 0x5000, object = 0x7000, vector = 0x8000;
    u32(base, buckets); u32(base + 4, entries);
    u32(base + 0x10, vector); u32(base + 0x14, vector);
    for (unsigned int i = 0; i < 4096; ++i) i16(buckets + i * 2, -1);
    /* Independently pinned hash bucket for guid>>8 == 1: 1440. */
    i16(buckets + 1440 * 2, 0);
    u32(entries, 2); i16(entries + 4, 1); i16(entries + 6, 0);
    u32(entries + 8, 1); i16(entries + 12, -1); i16(entries + 14, 0);
    u32(base + 0x20, object); u32(object + 0x1c, 0x10e);
    check(manager, 0x10e, COOP_ENTITY_PRESENT, object); /* collision */
    check(manager, 0x30e, COOP_ENTITY_ABSENT, 0);
    u32(object + 0x1c, 0x110);
    check(manager, 0x10e, COOP_ENTITY_GUID_MISMATCH, 0); /* reused generation */
    u32(object + 0x1c, 0x10e);
    valid[object + 0x1c] = 0;
    check(manager, 0x10e, COOP_ENTITY_INVALID, 0);
    valid[object + 0x1c] = 1;
    u32(base + 0x20, 0);
    check(manager, 0x10e, COOP_ENTITY_ABSENT, 0); /* cleared value */
    u32(base + 0x20, object);
    i16(entries + 4, 0);
    check(manager, 0x10e, COOP_ENTITY_INVALID, 0); /* cycle */
    i16(buckets + 1440 * 2, 6000);
    check(manager, 0x10e, COOP_ENTITY_INVALID, 0);
    i16(buckets + 1440 * 2, -1);
    u32(vector, object); u32(vector + 4, 1); u32(base + 0x14, vector + 8);
    check(manager, 0x10e, COOP_ENTITY_PRESENT, object); /* overflow fallback */
    u32(vector + 8, 0); u32(vector + 12, 1); u32(base + 0x14, vector + 16);
    check(manager, 0x10e, COOP_ENTITY_ABSENT, 0); /* reverse-order precedence */
    u32(base + 0x14, vector + 7);
    check(manager, 0x10e, COOP_ENTITY_INVALID, 0);
    u32(base + 0x14, vector + 4097 * 8);
    check(manager, 0x10e, COOP_ENTITY_INVALID, 0);
    u32(base + 0x14, vector - 8);
    check(manager, 0x10e, COOP_ENTITY_INVALID, 0);
    check(0xfffffff0, 0x10e, COOP_ENTITY_INVALID, 0);
    check(manager, 0, COOP_ENTITY_INVALID, 0);
    valid[base] = 0;
    check(manager, 0x10e, COOP_ENTITY_INVALID, 0);
    puts("PASS production registry: hash/collision, absence, generation reuse, missing memory, cycle, bounds, overflow precedence");
    return 0;
}
