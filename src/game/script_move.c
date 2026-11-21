/* Pinned ScrMoveV candidate. No default or diagnostic callback calls this. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "script_move.h"
#include "human_ai.h"
#include <math.h>
#include <string.h>
typedef void *(__attribute__((thiscall)) *ScrMove)(void *, uint32_t *,
    const float *, unsigned int, const float *, int);
typedef void (__attribute__((thiscall)) *ReleaseRef)(uint32_t *);
_Static_assert(sizeof(void *) == 4, "Pinned engine requires x86");
static int read_at(void *context, uint32_t at, void *out, size_t size) {
    (void)context;
    MEMORY_BASIC_INFORMATION r;
    if (!at || VirtualQuery((void *)(uintptr_t)at, &r, sizeof(r)) != sizeof(r) ||
        r.State != MEM_COMMIT || (r.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    uintptr_t end = (uintptr_t)r.BaseAddress + r.RegionSize;
    if (end < at || size > end - at) return 0;
    memcpy(out, (void *)(uintptr_t)at, size); return 1;
}
static int abi(void) {
    unsigned char start[9], release[10];
    static const unsigned char expected_start[] = {0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x34};
    static const unsigned char expected_release[] = {0x8b,9,0x85,0xc9,0x74,0x0e,0x83,0x41,4,0xff};
    return read_at(NULL, 0x9910b0, start, sizeof(start)) &&
        !memcmp(start, expected_start, sizeof(start)) &&
        read_at(NULL, 0x422d40, release, sizeof(release)) &&
        !memcmp(release, expected_release, sizeof(release));
}
static int sync_header(CoopScriptMove *o) {
    uint32_t h[2];
    return read_at(NULL, o->sync, h, sizeof(h)) && h[0] == 0x190cc0c &&
        h[1] && h[1] < 0x7fffffffu;
}
static int vector(const float p[3]) {
    if (!p) return 0;
    for (unsigned int i = 0; i < 3; ++i)
        if (!isfinite(p[i]) || fabsf(p[i]) >= 1000000.0f) return 0;
    return 1;
}
static CoopScriptMoveResult start(CoopScriptMove *o, const float p[3],
    unsigned int mode, const float direction[3]) {
    if (!o->guard(o->context, o->guid, o->human, COOP_SCRIPT_MOVE_START) || !abi())
        return COOP_SCRIPT_MOVE_REFUSED;
    CoopHumanAi ai;
    uint32_t script, script_owner, flags, agent_flags;
    if (CoopReadHumanAi(read_at, NULL, o->human, o->guid, &ai) != COOP_AI_VALID ||
        !ai.agent || !read_at(NULL, o->human+0x20, &flags, 4) || !(flags & 1) ||
        ai.agent > UINT32_MAX-4 || !read_at(NULL, ai.agent+4, &agent_flags, 4) ||
        !(agent_flags & 1) || !read_at(NULL, o->human+0xa4, &script, 4) ||
        !read_at(NULL, script, &script_owner, 4) || script_owner != o->human)
        return COOP_SCRIPT_MOVE_REFUSED;
    /* Native function constructs one out ref, owns temporary AI/sync refs and
       supplies the correct two-word AI payload to NewCommand. Do not pass a
       raw MoveDir here or manipulate its queue/free list. Internal OOM handling
       remains unaudited at runtime, so this is not an enabled backend. */
    ((ScrMove)(uintptr_t)0x9910b0)((void *)(uintptr_t)script,
        &o->sync, p, mode, direction, 1);
    return sync_header(o) ? COOP_SCRIPT_MOVE_HANDLE_CREATED : COOP_SCRIPT_MOVE_INCOMPLETE;
}
CoopScriptMoveResult CoopScriptMoveStart(CoopScriptMove *o,
    CoopScriptMoveGuard guard, void *context, uint32_t guid, uint32_t human,
    const float position[3], unsigned int mode, const float end_direction[3]) {
    if (!o || o->busy || o->sync || !guard || !guid || !human ||
        human > UINT32_MAX-0xac || !vector(position) || !vector(end_direction) || mode > 4)
        return COOP_SCRIPT_MOVE_REFUSED;
    o->busy = 1; o->guard = guard; o->context = context; o->guid = guid; o->human = human;
    CoopScriptMoveResult result = start(o, position, mode, end_direction);
    o->busy = 0; return result;
}
CoopScriptMoveResult CoopScriptMoveRelease(CoopScriptMove *o) {
    if (!o || o->busy || !o->sync || !o->guard) return COOP_SCRIPT_MOVE_REFUSED;
    o->busy = 1;
    CoopScriptMoveResult result = COOP_SCRIPT_MOVE_REFUSED;
    if (o->guard(o->context, o->guid, o->human, COOP_SCRIPT_MOVE_RELEASE) && abi() && sync_header(o)) {
        uint32_t owned = o->sync;
        o->sync = 0; /* Transfer before native destructor callbacks. */
        ((ReleaseRef)(uintptr_t)0x422d40)(&owned);
        result = COOP_SCRIPT_MOVE_REFERENCE_RELEASED;
    }
    o->busy = 0; return result;
}
