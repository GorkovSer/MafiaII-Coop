#ifndef COOP_ENTITY_REGISTRY_H
#define COOP_ENTITY_REGISTRY_H
#include <stddef.h>
#include <stdint.h>
/* Read-only pinned 32-bit registry layout; caller enforces image/thread identity. */
typedef int (*CoopRegistryRead)(void *context, uint32_t address, void *out, size_t size);
typedef enum {
    COOP_ENTITY_INVALID = 0,
    COOP_ENTITY_ABSENT,
    COOP_ENTITY_PRESENT,
    COOP_ENTITY_GUID_MISMATCH
} CoopEntityStatus;
CoopEntityStatus CoopEntityLookup(CoopRegistryRead read, void *context,
    uint32_t manager, uint32_t guid, uint32_t *pointer);
#endif
