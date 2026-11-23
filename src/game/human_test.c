/* One explicitly requested local Human lifecycle test. Not a network player.
   Called below the observer's EXE hash gate, only on game callbacks. */
#define WIN32_LEAN_AND_MEAN
#include "human_test.h"
#include "entity_registry.h"
#include "command_queue.h"
#include "human_ai.h"
#include "script_move.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <math.h>

int CoopProfileWasRedirected(void);
static wchar_t evidence[MAX_PATH];
static int enabled, attempted, initialized, loaded, paused;
static void *owned;
static uint32_t owned_guid;
static ULONGLONG remove_at;
static ULONGLONG ready_after;
static int position_reported;
static uint32_t pending_removal;
static ULONGLONG removal_deadline;
static int removal_status = -1;
static int script_enabled, script_attempted, script_end_requested;
static DWORD script_thread;
static ULONGLONG script_begin, script_next_sample;
static CoopScriptMove script_owner;
typedef void *(__attribute__((stdcall)) *model_factory_type)(void *);
typedef void *(__attribute__((cdecl)) *entity_factory_type)(unsigned int);
typedef int (__attribute__((thiscall)) *clone_type)(void *, void *);
typedef unsigned int (__attribute__((thiscall)) *release_model_type)(void *);
typedef void *(__attribute__((thiscall)) *set_model_type)(void *, void *);
typedef void (__attribute__((thiscall)) *entity_call_type)(void *);
typedef void (__attribute__((thiscall)) *set_position_type)(void *, const float *);

static int readable(const void *pointer, size_t size) {
    MEMORY_BASIC_INFORMATION region;
    if (!pointer || VirtualQuery(pointer, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT || (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    uintptr_t at = (uintptr_t)pointer, end = (uintptr_t)region.BaseAddress + region.RegionSize;
    return end >= at && size <= end - at;
}
static void record(const char *stage, unsigned int value) {
    char line[160];
    int n = snprintf(line, sizeof(line), "human_test=%s value=%u thread=%lu\r\n",
                     stage, value, (unsigned long)GetCurrentThreadId());
    if (n <= 0 || (size_t)n >= sizeof(line)) return;
    HANDLE file = CreateFileW(evidence, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written; WriteFile(file, line, (DWORD)n, &written, NULL); CloseHandle(file);
}
static int signature(uintptr_t at, const unsigned char *bytes, size_t size) {
    return readable((void *)at, size) && !memcmp((void *)at, bytes, size);
}
static int registry_read(void *context, uint32_t at, void *out, size_t size) {
    (void)context;
    if (!readable((void *)(uintptr_t)at, size)) return 0;
    memcpy(out, (void *)(uintptr_t)at, size); return 1;
}
static CoopEntityStatus resolve_guid(uint32_t guid, uint32_t *pointer) {
    uint32_t manager, table, lookup;
    *pointer = 0;
    if (!registry_read(NULL, 0x1abe8d8, &manager, 4) || !manager ||
        !registry_read(NULL, manager, &table, 4) || !table || table > UINT32_MAX - 0x1c ||
        !registry_read(NULL, table + 0x1c, &lookup, 4) || lookup != 0x5a0c70)
        return COOP_ENTITY_INVALID;
    return CoopEntityLookup(registry_read, NULL, manager, guid, pointer);
}
static int human_valid(void *human) {
    uint32_t current;
    return resolve_guid(owned_guid, &current) == COOP_ENTITY_PRESENT &&
        (uintptr_t)human == current && readable(human, 0x220) && *(uintptr_t *)human == 0x1967058 &&
        *(uint32_t *)((char *)human + 0x1c) == owned_guid;
}
static void record_command_queue(void *human) {
    if (!human_valid(human)) { record("command_queue_lost_handle", 1); return; }
    CoopCommandQueue queue;
    CoopCommandQueueStatus status = CoopReadCommandQueue(registry_read, NULL, (uint32_t)(uintptr_t)human, &queue);
    record("command_queue_status", (unsigned int)status);
    if (status == COOP_COMMAND_QUEUE_VALID) {
        record("command_queue_free", queue.free_count);
        record("command_queue_used", queue.used_count);
        record("command_retained_present", queue.retained_command != 0);
        record("command_retained_queued", queue.retained_is_queued);
        record("command_dispatch_skip_flag", !!(*(uint32_t *)((char *)human + 0x20) & 0x200000u));
    }
}
static void record_ai(void *human, const char *phase) {
    if (!human_valid(human)) { record("ai_lost_handle", 1); return; }
    CoopHumanAi ai;
    CoopHumanAiStatus status = CoopReadHumanAi(registry_read, NULL,
        (uint32_t)(uintptr_t)human, owned_guid, &ai);
    record(phase, (unsigned int)status);
    if (status == COOP_AI_VALID) {
        record("ai_agent_present", ai.agent != 0);
        record("ai_mode_flags_raw", ai.mode_flags);
    }
}
static void poll_removal(void) {
    if (!pending_removal) return;
    uint32_t current;
    CoopEntityStatus status = resolve_guid(pending_removal, &current);
    if ((int)status != removal_status) {
        removal_status = (int)status; record("removal_registry_status", (unsigned int)status);
    }
    if (status == COOP_ENTITY_ABSENT) {
        pending_removal = 0; record("removal_registry_confirmed", 1);
    } else if (GetTickCount64() >= removal_deadline) {
        pending_removal = 0; record("removal_registry_timeout_incomplete", 1);
    }
}
static void record_position(void *human) {
    if (!human_valid(human)) return;
    void *physics = *(void **)((char *)human + 0x190);
    void *model = *(void **)((char *)human + 0x60);
    if (!readable(physics, 0x1e4) || !readable(model, 0x68)) return;
    const float *p = (const float *)((char *)physics + 0x1d8);
    char line[256];
    int n = snprintf(line, sizeof(line),
        "human_position physics=%.5f,%.5f,%.5f model=%.5f,%.5f,%.5f\r\n",
        (double)p[0], (double)p[1], (double)p[2],
        (double)*(float *)((char *)model + 0x44), (double)*(float *)((char *)model + 0x54),
        (double)*(float *)((char *)model + 0x64));
    HANDLE file = CreateFileW(evidence, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        DWORD written; if (n > 0 && (size_t)n < sizeof(line)) WriteFile(file, line, (DWORD)n, &written, NULL);
        CloseHandle(file);
    }
}
/* Explicit isolated, offline native acceptance test. This guard grants no
   network/world authority and is never used by the remote actor backend. */
static int script_guard(void *context, uint32_t guid, uint32_t human, unsigned int op) {
    (void)context;
    if (!script_enabled || GetCurrentThreadId() != script_thread ||
        (uintptr_t)GetModuleHandleW(NULL) != 0x400000 || !CoopProfileWasRedirected()) return 0;
    if (op != COOP_SCRIPT_MOVE_START) return 1; /* Owned Sync only; no Human read. */
    return initialized && loaded && !paused && owned && guid == owned_guid &&
        human == (uint32_t)(uintptr_t)owned && human_valid(owned);
}
static void script_dispatch_sample(void) {
    CoopCommandQueue queue;
    if (!human_valid(owned) || CoopReadCommandQueue(registry_read, NULL,
        (uint32_t)(uintptr_t)owned, &queue) != COOP_COMMAND_QUEUE_VALID) return;
    record("script_queue_used", queue.used_count);
    if (!queue.retained_command) { record("script_movedir_retained", 0); return; }
    unsigned char a[0x58], b[0x58];
    if (!registry_read(NULL, queue.retained_command, a, sizeof(a)) ||
        !registry_read(NULL, queue.retained_command, b, sizeof(b)) ||
        memcmp(a, b, sizeof(a))) return;
    uint32_t type, refs;
    memcpy(&type, a, 4); memcpy(&refs, a+4, 4);
    if (type != 0x18f14b0 || !refs || refs >= 0x7fffffffu) {
        record("script_movedir_retained", 0); return;
    }
    float motion[2], multiplier;
    memcpy(motion, a+0x30, sizeof(motion)); memcpy(&multiplier, a+0x3c, 4);
    if (!isfinite(motion[0]) || !isfinite(motion[1]) || !isfinite(multiplier)) return;
    char line[180];
    int n = snprintf(line, sizeof(line),
        "human_script_dispatch used=%u queued=%u motion=%.5f,%.5f multiplier=%.5f\r\n",
        queue.used_count, queue.retained_is_queued, (double)motion[0], (double)motion[1], (double)multiplier);
    HANDLE file = CreateFileW(evidence, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        DWORD written; if (n > 0 && (size_t)n < sizeof(line)) WriteFile(file, line, (DWORD)n, &written, NULL);
        CloseHandle(file);
    }
}
static void script_tick(void) {
    if (!script_enabled || !owned || !human_valid(owned)) return;
    ULONGLONG now = GetTickCount64();
    if (!script_attempted) {
        if (now + 55000 < remove_at) return;
        script_attempted = 1; script_thread = GetCurrentThreadId();
        void *physics = *(void **)((char *)owned + 0x190);
        if (!readable(physics, 0x1e4)) { record("script_refused_physics", 1); return; }
        float target[3], direction[3] = {0, 0, 0};
        memcpy(target, (char *)physics+0x1d8, sizeof(target));
        target[0] += 6.0f;
        record("script_start_begin", 1);
        CoopScriptMoveResult result = CoopScriptMoveStart(&script_owner, script_guard,
            NULL, owned_guid, (uint32_t)(uintptr_t)owned, target, 2, direction);
        record("script_start_result", (unsigned int)result);
        script_begin = now; script_next_sample = now;
    }
    if (!script_owner.sync) return;
    if (!script_end_requested && now-script_begin >= 1500) {
        script_end_requested = 1;
        record("script_end_result", (unsigned int)CoopScriptMoveRequestEnd(&script_owner));
    }
    if (now >= script_next_sample && now-script_begin <= 12000) {
        script_next_sample = now+200;
        record("script_elapsed_ms", (unsigned int)(now-script_begin));
        CoopScriptState state;
        CoopScriptStateStatus status = CoopScriptMoveObserve(&script_owner, &state);
        record("script_snapshot_status", (unsigned int)status);
        if (status == COOP_SCRIPT_STATE_VALID) {
            record("script_command_state", state.command_state);
            record("script_abort_byte", state.command_flag_d);
            record("script_sync_flags", state.sync_flags);
            record("script_link_present", state.linked_to_sync);
            record("script_command_references", state.command_references);
        }
        record_position(owned);
        script_dispatch_sample();
    }
}
static void remove_owned(void) {
    void *human = owned;
    owned = NULL; /* Engine may delete immediately: abandon handle BEFORE call. */
    if (!human) return;
    if (!human_valid(human)) { record("lost_handle_no_release", 1); return; }
    if (script_owner.sync)
        record("script_retire_end", (unsigned int)CoopScriptMoveRequestEnd(&script_owner));
    pending_removal = owned_guid;
    removal_deadline = GetTickCount64() + 10000;
    removal_status = -1;
    record_ai(human, "ai_before_release");
    record("release_requested", 1);
    ((entity_call_type)(uintptr_t)0x119ddd0)(human);
    /* No read of human or its model after Release, no second destructor/free. */
    record("release_returned", 1);
    if (script_owner.sync)
        record("script_ref_release", (unsigned int)CoopScriptMoveRelease(&script_owner));
    poll_removal();
}
int CoopHumanTestStart(const wchar_t *path) {
    if (!path || wcslen(path) >= MAX_PATH) return 1;
    wcscpy(evidence, path);
    wchar_t ini[MAX_PATH]; wcscpy(ini, path);
    wchar_t *tail = wcsrchr(ini, L'\\'); if (!tail) return 1;
    *tail = 0;
    if (wcslen(ini) + wcslen(L"\\human-test.ini") >= MAX_PATH) return 1;
    wcscat(ini, L"\\human-test.ini");
    enabled = GetPrivateProfileIntW(L"test", L"human_once", 0, ini) == 1;
    script_enabled = GetPrivateProfileIntW(L"test", L"script_move_once", 0, ini) == 1;
    /* Redirection is observed when the game first queries its profile, after
       this early export. Creation and every script guard require it later. */
    if (script_enabled && !enabled) return 3;
    if (script_enabled) {
        *tail = 0;
        if (wcslen(ini)+wcslen(L"\\coop-session.ini") >= MAX_PATH) return 3;
        wcscat(ini, L"\\coop-session.ini");
        if (GetFileAttributesW(ini) != INVALID_FILE_ATTRIBUTES) return 3;
        record("script_offline_diagnostic", 1);
    }
    if (!enabled) return 0;
    static const unsigned char factory[] = {0x68,0x20,0x02,0,0,0xe8,0xc6,0x7a,0xa6,0xff};
    static const unsigned char setup[] = {0x83,0xec,0x50,0x53,0x55,0x56,0x57,0x8b,0xe9};
    static const unsigned char position[] = {0x56,0x8b,0xf1,0x83,0xbe,0x90,0x01,0,0,0};
    static const unsigned char activate[] = {0x56,0x8b,0xf1,0xf6,0x46,0x20,0x20};
    if (!signature(0x999d60, factory, sizeof(factory)) ||
        !signature(0x99f400, setup, sizeof(setup)) ||
        !signature(0x94e960, position, sizeof(position)) ||
        !signature(0x1192170, activate, sizeof(activate))) return 2;
    record("enabled", 1); return 0;
}
void CoopHumanTestEvent(unsigned int id) {
    if (!enabled) return;
    poll_removal();
    if (id == 11 || id == 14 || id == 32 || id == 39) remove_owned();
    if (id == 10 || id == 11 || id == 12 || id == 14 || id == 39) initialized = loaded = 0;
    if (id == 13) initialized = 1;
    if (id == 32) { loaded = 0; ready_after = 0; }
    if (id == 33) loaded = 1;
    if (id == 34 || id == 4) paused = 1;
    if (id == 35) paused = 0;
    if (owned && (id == 3 || id == 4) && GetTickCount64() >= remove_at) remove_owned();
    if (owned && id == 3 && !position_reported && GetTickCount64() + 55000 >= remove_at) {
        position_reported = 1; record_position(owned); record_command_queue(owned);
    }
    if (owned && id == 3 && loaded && initialized && !paused) script_tick();
    if (attempted || id != 3 || !initialized || !loaded || paused) return;
    if (!CoopProfileWasRedirected()) { attempted = 1; record("refused_profile", 0); return; }
    void *game = *(void **)(uintptr_t)0x1abfe14;
    if (!readable(game, 0x11c)) return;
    void *player = *(void **)((char *)game + 0x118);
    if (!readable(player, 0x194)) return;
    void *source = *(void **)((char *)player + 0x60);
    if (!readable(source, 0xa0) || *(uintptr_t *)source != 0x191c850) return;
    /* load_finished precedes the checkpoint's final placement/visual settling.
       Use the audited physics position, not an early model/template transform. */
    void *physics = *(void **)((char *)player + 0x190);
    if (!readable(physics, 0x1e4)) return;
    uintptr_t physics_table = *(uintptr_t *)physics;
    if (!readable((void *)physics_table, 0x58) ||
        *(uintptr_t *)(physics_table + 0x54) != 0x18ae690) return;
    if (!ready_after) { ready_after = GetTickCount64() + 3000; return; }
    if (GetTickCount64() < ready_after) return;
    float position[3];
    for (unsigned int i = 0; i < 3; ++i) {
        position[i] = ((float *)((char *)physics + 0x1d8))[i];
        if (!isfinite(position[i]) || fabsf(position[i]) >= 1000000.0f) return;
    }
    position[0] -= 2.0f; position[1] += 2.0f;
    attempted = 1;
    void *model = ((model_factory_type)(uintptr_t)0x14ec940)(NULL);
    if (!model) { record("model_allocation_failed", 0); return; }
    int clone_status = ((clone_type)(uintptr_t)0x14ec8f0)(model, source);
    if (clone_status < 0) {
        ((release_model_type)(uintptr_t)0x14e5bc0)(model);
        record("clone_failed", (unsigned int)clone_status); return;
    }
    record("clone_ready", 1);
    void *human = ((entity_factory_type)(uintptr_t)0x1167a60)(0x0e);
    if (!human) {
        ((release_model_type)(uintptr_t)0x14e5bc0)(model);
        record("entity_allocation_failed", 0); return;
    }
    if (!readable(human, 0x220) || *(uintptr_t *)human != 0x1967058 ||
        *(void **)((char *)human + 0x60) != NULL) {
        ((release_model_type)(uintptr_t)0x14e5bc0)(model);
        record("unexpected_entity_contract", 0); return;
    }
    owned = human; owned_guid = *(uint32_t *)((char *)human + 0x1c);
    if (!human_valid(human)) {
        owned = NULL;
        ((release_model_type)(uintptr_t)0x14e5bc0)(model);
        record("unregistered_entity_incomplete", 1); return;
    }
    record("creation_registry_confirmed", 1);
    record("created", owned_guid);
    void *transferred_model = model; /* Address comparison only after transfer. */
    ((set_model_type)(uintptr_t)0x1164d00)(human, model);
    *(uint32_t *)((char *)human + 0x20) |= 0x800; /* Transfer exactly one owned model ref. */
    model = NULL;
    ((set_position_type)(uintptr_t)0x94e960)(human, position);
    record("setup_begin", 1);
    ((entity_call_type)(uintptr_t)0x99f400)(human);
    record("setup_returned", 1);
    record("setup_model_replaced", *(void **)((char *)human + 0x60) != transferred_model);
    if (!(*(uint32_t *)((char *)human + 0x20) & 0x20) ||
        !*(void **)((char *)human + 0x190)) {
        record("setup_incomplete", 0); remove_owned(); return;
    }
    ((set_position_type)(uintptr_t)0x94e960)(human, position);
    record("flags_after_setup", *(uint32_t *)((char *)human + 0x20));
    record_ai(human, "ai_after_setup");
    /* Upstream Human/Player wrappers clear exactly this bit after Setup.
       Its semantic name is not established; this is a measured experiment. */
    *(uint32_t *)((char *)human + 0x20) &= ~0x40u;
    ((entity_call_type)(uintptr_t)0x1192170)(human);
    record("active", !!(*(uint32_t *)((char *)human + 0x20) & 1));
    record_ai(human, "ai_after_activate");
    remove_at = GetTickCount64() + 60000;
    record_position(human);
    record_command_queue(human);
}
