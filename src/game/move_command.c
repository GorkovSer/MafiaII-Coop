/* Pinned x86 native command candidate. No initializer or game callback calls
   these functions. Runtime allocation/ABI/control acceptance is still pending. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "move_command.h"
#include "command_queue.h"
#include <math.h>
#include <string.h>

typedef void *(__attribute__((cdecl)) *Allocate)(unsigned int);
typedef void *(__attribute__((thiscall)) *ConstructRef)(void *, void *);
typedef void (__attribute__((thiscall)) *ReleaseRef)(void *);
typedef void (__attribute__((thiscall)) *AddCommand)(void *, unsigned int, uint32_t);
typedef void (__attribute__((thiscall)) *ClearMove)(void *);
typedef void (__attribute__((thiscall)) *InitializeMove)(void *);
_Static_assert(sizeof(void *) == 4, "Pinned game is x86");

static int accessible(uint32_t at, size_t size, int write) {
    MEMORY_BASIC_INFORMATION r;
    if (!at || VirtualQuery((void *)(uintptr_t)at, &r, sizeof(r)) != sizeof(r) ||
        r.State != MEM_COMMIT || (r.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    uintptr_t end = (uintptr_t)r.BaseAddress + r.RegionSize;
    DWORD p = r.Protect & 255;
    if (write && p != PAGE_READWRITE && p != PAGE_WRITECOPY &&
        p != PAGE_EXECUTE_READWRITE && p != PAGE_EXECUTE_WRITECOPY) return 0;
    return end >= at && size <= end - at;
}
static int read_at(void *context, uint32_t at, void *out, size_t size) {
    (void)context;
    if (!accessible(at, size, 0)) return 0;
    memcpy(out, (void *)(uintptr_t)at, size); return 1;
}
static int bytes(uint32_t at, const unsigned char *expected, size_t size) {
    return accessible(at, size, 0) && !memcmp((void *)(uintptr_t)at, expected, size);
}
static int abi(void) {
    static const unsigned char alloc[] = {0xb8,1,0,0,0,0x84,5,0xdc,0x2b,0xab,1};
    static const unsigned char ctor[] = {0x8b,0x44,0x24,4,0x56,0x8b,0xf1,0x89,6};
    static const unsigned char release[] = {0x8b,9,0x85,0xc9,0x74,0x0e,0x83,0x41,4,0xff};
    static const unsigned char enqueue[] = {0x8b,0x44,0x24,4,0x56,0x57,0x8b,0xf9,0x83,0xf8,0x0e};
    static const unsigned char clear[] = {0x83,0xec,0x0c,0x56,0x6a,0,0x8b,0xf1};
    static const unsigned char init[] = {0xf3,0x0f,0x10,5,0xf4,0x18,0x8f,1};
    uint32_t dtor, start, tick;
    return bytes(0x401830, alloc, sizeof(alloc)) && bytes(0x429a10, ctor, sizeof(ctor)) &&
        bytes(0x422db0, release, sizeof(release)) && bytes(0x94d400, enqueue, sizeof(enqueue)) &&
        bytes(0x94d8d0, clear, sizeof(clear)) &&
        bytes(0x936fb0, init, sizeof(init)) &&
        read_at(NULL, 0x18f14b0, &dtor, 4) && dtor == 0x42d3e0 &&
        read_at(NULL, 0x1cbebb0, &start, 4) && start == 0x9807a0 &&
        read_at(NULL, 0x1cbebb8, &tick, 4) && tick == 0x937050;
}
static int guarded(CoopMoveCommand *o, unsigned int operation) {
    return o && o->guard && o->guard(o->context, o->guid, o->human, operation) && abi();
}
static int header(CoopMoveCommand *o) {
    uint32_t h[4];
    return o->command && read_at(NULL, o->command, h, sizeof(h)) &&
        h[0] == 0x18f14b0 && h[1] && h[1] < 0x7fffffffu &&
        h[2] == 0x1cbebb0 && (h[3] & 255) == 1;
}
static int queue(CoopMoveCommand *o, CoopCommandQueue *q, int inserting) {
    if (CoopReadCommandQueue(read_at, NULL, o->human, q) != COOP_COMMAND_QUEUE_VALID) return 0;
    if (inserting && (!q->free_count || q->retained_command)) return 0;
    unsigned int matches = 0;
    for (unsigned int i = 0; i < 8; ++i) {
        if (q->slots[i].queue != 0 || !q->slots[i].command) continue;
        if (inserting || q->slots[i].command != o->command) return 0;
        ++matches;
    }
    return inserting || matches == 1;
}
static CoopMoveResult create(CoopMoveCommand *o, CoopMoveGuard guard,
    void *context, uint32_t guid, uint32_t human) {
    if (!o || o->command || !guard || !guid || !human) return COOP_MOVE_REFUSED;
    o->guard = guard; o->context = context; o->guid = guid; o->human = human;
    CoopCommandQueue q;
    if (!guarded(o, COOP_MOVE_EDIT) || !queue(o, &q, 1)) return COOP_MOVE_REFUSED;
    void *raw = ((Allocate)(uintptr_t)0x401830)(0x58);
    if (!raw) return COOP_MOVE_REFUSED; /* Never pass null to AddCommand. */
    /* Candidate base initialization mirrors the pinned native factory's class
       and size. Zero reserved storage rather than inheriting allocator bytes;
       AddCommand performs the engine's type-1 initialization. Not yet tested. */
    memset(raw, 0, 0x58);
    *(uint32_t *)raw = 0x18f14b0;
    ((ConstructRef)(uintptr_t)0x429a10)(&o->command, raw);
    ((InitializeMove)(uintptr_t)0x936fb0)(raw);
    /* Allocation/ref construction may reenter engine code. Recheck actor,
       authority and capacity before consuming an argument reference. */
    if (!guarded(o, COOP_MOVE_EDIT) || !header(o) || !queue(o, &q, 1)) return COOP_MOVE_INCOMPLETE;
    uint32_t argument = 0;
    ((ConstructRef)(uintptr_t)0x429a10)(&argument, raw);
    ((AddCommand)(uintptr_t)0x94d400)((void *)(uintptr_t)human, 1, argument);
    /* AddCommand consumes the reference represented by its stack copy.
       C has no automatic destructor for this local. Do not release it twice. */
    argument = 0;
    if (!header(o) || !queue(o, &q, 0)) return COOP_MOVE_INCOMPLETE;
    return COOP_MOVE_CREATED;
}
static CoopMoveResult input(CoopMoveCommand *o,
    const float facing[2], const float motion[2], float multiplier, uint8_t mode) {
    if (!facing || !motion || !isfinite(multiplier) || multiplier < 0 || multiplier > 1 || mode > 4)
        return COOP_MOVE_REFUSED;
    for (unsigned int i = 0; i < 2; ++i)
        if (!isfinite(facing[i]) || !isfinite(motion[i]) || fabsf(facing[i]) > 1 || fabsf(motion[i]) > 1)
            return COOP_MOVE_REFUSED;
    if (facing[0]*facing[0] + facing[1]*facing[1] > 1.001f ||
        motion[0]*motion[0] + motion[1]*motion[1] > 1.001f) return COOP_MOVE_REFUSED;
    CoopCommandQueue q;
    if (!guarded(o, COOP_MOVE_EDIT) || !header(o) || !queue(o, &q, 0) ||
        !accessible(o->command, 0x58, 1)) return COOP_MOVE_REFUSED;
    uint32_t flags;
    memcpy(&flags, (void *)(uintptr_t)(o->command+0xc), 4);
    if ((flags & 0x100) || !(flags & 0x400)) return COOP_MOVE_REFUSED;
    unsigned char *p = (unsigned char *)(uintptr_t)o->command;
    memcpy(p+0x28, facing, 8); memcpy(p+0x30, motion, 8);
    memcpy(p+0x3c, &multiplier, 4); p[0x50] = mode; /* preserve high control bits */
    return COOP_MOVE_UPDATED;
}
static CoopMoveResult cancel(CoopMoveCommand *o) {
    CoopCommandQueue q;
    if (!guarded(o, COOP_MOVE_CLEANUP) || !header(o) || !queue(o, &q, 0)) return COOP_MOVE_REFUSED;
    ((ClearMove)(uintptr_t)0x94d8d0)((void *)(uintptr_t)o->human);
    /* ClearMove does not clear Human+0x1a0 or establish physical stop. */
    return COOP_MOVE_CANCEL_REQUESTED;
}
static CoopMoveResult release(CoopMoveCommand *o) {
    if (!guarded(o, COOP_MOVE_RELEASE) || !header(o)) return COOP_MOVE_REFUSED;
    uint32_t owned = o->command;
    o->command = 0; /* Destructor may reenter: transfer before the call. */
    ((ReleaseRef)(uintptr_t)0x422db0)(&owned);
    return COOP_MOVE_RELEASED;
}

/* Hold across guards as well as native calls: either can reenter. In
   particular, allocator reentry precedes assigning command, and destructor
   reentry follows clearing it. Neither interval permits another creation. */
static int enter(CoopMoveCommand *o) {
    if (!o || o->busy) return 0;
    o->busy = 1;
    return 1;
}
CoopMoveResult CoopMoveCommandCreate(CoopMoveCommand *o, CoopMoveGuard guard,
    void *context, uint32_t guid, uint32_t human) {
    if (!enter(o)) return COOP_MOVE_REFUSED;
    CoopMoveResult result = create(o, guard, context, guid, human);
    o->busy = 0;
    return result;
}
CoopMoveResult CoopMoveCommandInput(CoopMoveCommand *o,
    const float facing[2], const float motion[2], float multiplier, uint8_t mode) {
    if (!enter(o)) return COOP_MOVE_REFUSED;
    CoopMoveResult result = input(o, facing, motion, multiplier, mode);
    o->busy = 0;
    return result;
}
CoopMoveResult CoopMoveCommandCancel(CoopMoveCommand *o) {
    if (!enter(o)) return COOP_MOVE_REFUSED;
    CoopMoveResult result = cancel(o);
    o->busy = 0;
    return result;
}
CoopMoveResult CoopMoveCommandRelease(CoopMoveCommand *o) {
    if (!enter(o)) return COOP_MOVE_REFUSED;
    CoopMoveResult result = release(o);
    o->busy = 0;
    return result;
}
