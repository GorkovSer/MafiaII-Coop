/* Read-only equivalent of 0x5a0c70 -> 0x59b130 -> 0x58ebe0.
   Invalid/unreadable/cyclic memory is never evidence of confirmed absence. */
#include "entity_registry.h"
#include <string.h>

static int take(CoopRegistryRead read, void *context, uint64_t at, void *out, size_t size) {
    return at && at <= UINT32_MAX && size <= UINT64_C(0x100000000) - at &&
        read(context, (uint32_t)at, out, size);
}
static CoopEntityStatus finish(CoopRegistryRead read, void *context,
    uint32_t candidate, uint32_t guid, uint32_t *pointer) {
    uint32_t actual;
    if (!candidate) return COOP_ENTITY_ABSENT;
    if (!take(read, context, (uint64_t)candidate + 0x1c, &actual, 4)) return COOP_ENTITY_INVALID;
    if (actual != guid) return COOP_ENTITY_GUID_MISMATCH;
    *pointer = candidate;
    return COOP_ENTITY_PRESENT;
}
CoopEntityStatus CoopEntityLookup(CoopRegistryRead read, void *context,
    uint32_t manager, uint32_t guid, uint32_t *pointer) {
    if (!pointer) return COOP_ENTITY_INVALID;
    *pointer = 0;
    if (!read || !manager || !guid) return COOP_ENTITY_INVALID;
    uint64_t base = (uint64_t)manager + 4;
    uint32_t buckets, entries;
    if (!take(read, context, base, &buckets, 4) || !buckets ||
        !take(read, context, base + 4, &entries, 4) || !entries) return COOP_ENTITY_INVALID;
    uint32_t key = guid >> 8;
    uint32_t mixed = ~(key << 15) + key;
    mixed ^= mixed >> 10;
    mixed *= 9;
    mixed ^= mixed >> 6;
    mixed += ~(mixed << 11);
    mixed ^= mixed >> 16;
    int16_t index;
    if (!take(read, context, (uint64_t)buckets + (mixed & 0xfff) * 2, &index, 2)) return COOP_ENTITY_INVALID;
    unsigned char seen[750] = {0};
    while (index >= 0) {
        unsigned int slot = (unsigned int)index;
        if (slot >= 6000 || (seen[slot / 8] & (1u << (slot % 8)))) return COOP_ENTITY_INVALID;
        seen[slot / 8] |= (unsigned char)(1u << (slot % 8));
        unsigned char entry[8]; uint32_t found;
        if (!take(read, context, (uint64_t)entries + slot * 8, entry, sizeof(entry))) return COOP_ENTITY_INVALID;
        memcpy(&found, entry, 4);
        memcpy(&index, entry + 4, 2);
        if (found == key) {
            uint32_t candidate;
            if (!take(read, context, base + 0x1c + slot * 4, &candidate, 4)) return COOP_ENTITY_INVALID;
            return finish(read, context, candidate, guid, pointer);
        }
    }
    uint32_t begin, end;
    if (!take(read, context, base + 0x10, &begin, 4) ||
        !take(read, context, base + 0x14, &end, 4) || end < begin ||
        (end - begin) % 8 || (end - begin) / 8 > 4096) return COOP_ENTITY_INVALID;
    for (uint32_t count = (end - begin) / 8; count; --count) {
        uint32_t entry[2];
        if (!take(read, context, (uint64_t)begin + (count - 1) * 8, entry, sizeof(entry))) return COOP_ENTITY_INVALID;
        if (entry[1] == key) return finish(read, context, entry[0], guid, pointer);
    }
    return COOP_ENTITY_ABSENT;
}
