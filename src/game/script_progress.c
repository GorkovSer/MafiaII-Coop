#include "script_progress.h"
#include <math.h>
#include <string.h>

typedef struct {
    uint32_t address;
    size_t size;
    unsigned char text[14];
} NameObservation;

static int take(CoopProgressRead read, void *context, uint32_t at, void *out, size_t size) {
    return at && size && (uint64_t)at + size <= (uint64_t)UINT32_MAX + 1 &&
        read(context, at, out, size);
}

CoopProgressStatus CoopReadScriptProgress(CoopProgressRead read, void *context,
    uint32_t manager_slot, CoopScriptProgress *out) {
    if (!out) return COOP_PROGRESS_UNREADABLE;
    memset(out, 0, sizeof(*out));
    if (!read) return COOP_PROGRESS_UNREADABLE;
    uint32_t manager, count;
    if (!take(read, context, manager_slot, &manager, 4) || !manager)
        return COOP_PROGRESS_UNREADABLE;
    if ((manager & 3) || manager > UINT32_MAX - 0x4b8u) return COOP_PROGRESS_BAD_LAYOUT;
    if (!take(read, context, manager + 0x4b4u, &count, 4)) return COOP_PROGRESS_UNREADABLE;
    if (count > 100) return COOP_PROGRESS_BAD_LAYOUT;
    if (!count) return COOP_PROGRESS_MISSING;
    unsigned char records[1200], verify[1200];
    size_t bytes = (size_t)count * 12;
    if (!take(read, context, manager + 4, records, bytes)) return COOP_PROGRESS_UNREADABLE;
    static const char *const keys[2] = {"missionNumber", "missionPart"};
    static const size_t lengths[2] = {14, 12};
    NameObservation names[200];
    size_t observations = 0;
    uint32_t found = 0, values[2] = {0, 0};
    /* Match native reverse lookup, including priority of the newest duplicate.
       Wrong type/value on that entry must not fall back to an older match. */
    for (uint32_t i = count; i && found != 3; --i) {
        const unsigned char *record = records + (i - 1) * 12;
        uint32_t offset;
        uint16_t kind;
        float value;
        memcpy(&offset, record, 4);
        memcpy(&value, record + 4, 4);
        memcpy(&kind, record + 8, 2);
        for (unsigned int key = 0; key < 2; ++key) {
            if (found & (1u << key)) continue;
            size_t length = lengths[key];
            uint64_t address = (uint64_t)manager + 0x4b8u + offset;
            /* 4096 is a conservative diagnostic budget, not engine pool size. */
            if (offset > 4096 - length || address + length > (uint64_t)UINT32_MAX + 1)
                return COOP_PROGRESS_BAD_LAYOUT;
            NameObservation *name = &names[observations++];
            name->address = (uint32_t)address;
            name->size = length;
            if (!take(read, context, name->address, name->text, length))
                return COOP_PROGRESS_UNREADABLE;
            if (memcmp(name->text, keys[key], length)) continue;
            if (kind != 2 || !isfinite(value) || value < 0 || value > 65535 ||
                (float)(uint32_t)value != value) return COOP_PROGRESS_BAD_VALUE;
            values[key] = (uint32_t)value;
            found |= 1u << key;
        }
    }
    uint32_t current_manager, current_count;
    if (!take(read, context, manager_slot, &current_manager, 4) || current_manager != manager ||
        !take(read, context, manager + 0x4b4u, &current_count, 4) || current_count != count ||
        !take(read, context, manager + 4, verify, bytes) || memcmp(records, verify, bytes))
        return COOP_PROGRESS_CHANGED;
    for (size_t i = 0; i < observations; ++i) {
        unsigned char text[14];
        if (!take(read, context, names[i].address, text, names[i].size) ||
            memcmp(text, names[i].text, names[i].size)) return COOP_PROGRESS_CHANGED;
    }
    if (found != 3) return COOP_PROGRESS_MISSING;
    out->mission_number = values[0];
    out->mission_part = values[1];
    return COOP_PROGRESS_OBSERVED;
}
