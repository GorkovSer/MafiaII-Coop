#ifndef COOP_SCRIPT_PROGRESS_H
#define COOP_SCRIPT_PROGRESS_H
#include <stddef.h>
#include <stdint.h>

typedef int (*CoopProgressRead)(void *context, uint32_t address, void *out, size_t size);
typedef enum {
    COOP_PROGRESS_UNREADABLE = 0,
    COOP_PROGRESS_MISSING,
    COOP_PROGRESS_BAD_LAYOUT,
    COOP_PROGRESS_BAD_VALUE,
    COOP_PROGRESS_CHANGED,
    COOP_PROGRESS_OBSERVED
} CoopProgressStatus;

typedef struct {
    uint32_t mission_number, mission_part;
} CoopScriptProgress;

/* Pinned x86 script-variable layout. Caller supplies the singleton slot, not
   the object. Read-only diagnostic: OBSERVED does not prove current world,
   checkpoint identity, load completion or lifecycle freshness. No caching or
   engine calls. Output is zero on all non-OBSERVED results. */
CoopProgressStatus CoopReadScriptProgress(CoopProgressRead read, void *context,
    uint32_t manager_slot, CoopScriptProgress *out);
#endif
