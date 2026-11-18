/* Minimal campaign-preserving event observer for one audited GOG executable.
   SDK evidence and checked calling convention are documented in RESEARCH.md.
   No bulk SDK initialization, world startup, NPC or save manipulation. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../game/telemetry.h"
#include "../game/model_test.h"
#include "../game/human_test.h"

static wchar_t log_path[MAX_PATH];
static const unsigned char original[5] = {0x8b, 0x42, 0x1c, 0x6a, 0x0a};
static volatile LONG registration_started;
__attribute__((used)) static uintptr_t observer_resume = 0x4f2f0a;

static void record(const char *event, unsigned long a, unsigned long b) {
    char text[256];
    int n = snprintf(text, sizeof(text), "observer=%s value=%lu thread=%lu detail=%lu\r\n",
        event, a, (unsigned long)GetCurrentThreadId(), b);
    if (n <= 0 || (size_t)n >= sizeof(text)) return;
    HANDLE file = CreateFileW(log_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(file, text, (DWORD)n, &written, NULL);
    CloseHandle(file);
}

static int audited_image(void) {
    static const unsigned char expected[32] = {
        0x16,0xc7,0x9a,0x3b,0xe4,0x83,0xe5,0x82,0x71,0x9e,0x43,0xd7,0xb4,0x51,0xe8,0x87,
        0xe8,0x94,0x14,0xbe,0x81,0xca,0x7e,0xcf,0xc0,0x53,0x6d,0xd8,0x18,0x5e,0x6c,0x2f};
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!n || n >= MAX_PATH) return 0;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    unsigned char bytes[8192], digest[32];
    DWORD count = 0, digest_size = sizeof(digest);
    int ok = CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
        CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash);
    while (ok) {
        if (!ReadFile(file, bytes, sizeof(bytes), &count, NULL)) { ok = 0; break; }
        if (!count) break;
        ok = CryptHashData(hash, bytes, count, 0);
    }
    if (ok) ok = CryptGetHashParam(hash, HP_HASHVAL, digest, &digest_size, 0) &&
        digest_size == sizeof(expected) && !memcmp(digest, expected, sizeof(expected));
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider, 0);
    CloseHandle(file);
    return ok;
}

static int readable(const void *pointer, size_t bytes) {
    MEMORY_BASIC_INFORMATION region;
    if (VirtualQuery(pointer, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return 0;
    uintptr_t at = (uintptr_t)pointer, end = (uintptr_t)region.BaseAddress + region.RegionSize;
    return end >= at && bytes <= end - at;
}

static int write_startup_patch(const void *bytes) {
    void *target = (void *)(uintptr_t)0x4f2f05;
    DWORD old, ignored;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old)) return 0;
    memcpy(target, bytes, 5);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return VirtualProtect(target, 5, old, &ignored) != 0;
}

typedef struct { const char *name; LONG count; unsigned int id; } observation;
typedef void (__attribute__((thiscall)) *callback_type)(void *, void *);
typedef unsigned char (__attribute__((thiscall)) *add_action_type)(
    void *, unsigned int, int, void *, callback_type, float, unsigned int, unsigned int);

static void __attribute__((thiscall, force_align_arg_pointer)) on_event(void *self, void *context) {
    (void)context;
    observation *item = (observation *)self;
    CoopTelemetryEvent(item->id);
    CoopModelTestEvent(item->id);
    CoopHumanTestEvent(item->id);
    LONG count = InterlockedIncrement(&item->count);
    /* Every lifecycle transition matters, including later menu returns and
       pauses. Frame callbacks still log only once; no per-frame disk I/O. */
    int frame = item->id == 3 || item->id == 4 || item->id == 5 || item->id == 7 ||
        item->id == 22 || item->id == 23 || item->id == 24;
    if (count == 1 || !frame) record(item->name, (unsigned long)count, 0);
}

#define EVENT(id, name) {id, {name, 0, id}}
static struct { unsigned int id; observation item; } events[] = {
    EVENT(3, "tick"), EVENT(4, "tick_paused"), EVENT(5, "tick_always"),
    EVENT(7, "render"), EVENT(9, "mission_before_open"), EVENT(10, "mission_open"),
    EVENT(11, "mission_before_close"), EVENT(12, "mission_close"),
    EVENT(13, "game_init"), EVENT(14, "game_done"), EVENT(19, "game_restore"),
    EVENT(20, "no_game_start"), EVENT(21, "no_game_end"),
    EVENT(22, "no_game_tick"), EVENT(23, "no_game_render"),
    EVENT(24, "gameloop_after"), EVENT(27, "game_save"),
    EVENT(32, "load_started"), EVENT(33, "load_finished"),
    EVENT(34, "game_paused"), EVENT(35, "game_unpaused"), EVENT(39, "shutdown")
};
#undef EVENT

__attribute__((used, noinline)) static void observer_register(void) {
    if (InterlockedCompareExchange(&registration_started, 1, 0)) return;
    record("startup_hook_restored", (unsigned long)write_startup_patch(original), 0);
    const void *global = (void *)(uintptr_t)0x1abe8bc;
    if (!readable(global, 4)) { record("manager_unreadable", 0, 0); return; }
    void *manager = *(void *const *)global;
    if (!manager || !readable(manager, 16)) { record("manager_unavailable", 0, 0); return; }
    add_action_type add_action = (add_action_type)(uintptr_t)0x11a0c10;
    unsigned int accepted = 0;
    for (unsigned int i = 0; i < sizeof(events) / sizeof(events[0]); ++i) {
        unsigned char ok = add_action(manager, events[i].id, 400, &events[i].item,
                                      on_event, -1.0f, 0, 0);
        accepted += ok != 0;
        record("add_action", events[i].id, ok);
    }
    record("registered", accepted, (unsigned long)(sizeof(events) / sizeof(events[0])));
}

__attribute__((naked)) static void startup_bridge(void) {
    __asm__ volatile (
        "pushfl\n\tpushal\n\tmovl %esp, %ebp\n\tandl $-16, %esp\n\t"
        "call _observer_register\n\tmovl %ebp, %esp\n\tpopal\n\tpopfl\n\t"
        "movl 0x1c(%edx), %eax\n\tpushl $0x0a\n\tjmp *_observer_resume\n\t");
}

DWORD CoopObserverInstall(const wchar_t *path) {
    if (!path || wcslen(path) >= MAX_PATH) return 10;
    wcscpy(log_path, path);
    if ((uintptr_t)GetModuleHandleW(NULL) != 0x400000 || !audited_image()) {
        record("refused_unknown_image", 0, 0); return 11;
    }
    const void *target = (void *)(uintptr_t)0x4f2f05;
    const void *add = (void *)(uintptr_t)0x11a0c10;
    static const unsigned char add_prefix[] = {0x83,0xec,0x20,0x53,0x8b,0x5c,0x24,0x30};
    if (!readable(target, sizeof(original)) || memcmp(target, original, sizeof(original)) ||
        !readable(add, sizeof(add_prefix)) || memcmp(add, add_prefix, sizeof(add_prefix))) {
        record("refused_signature_mismatch", 0, 0); return 12;
    }
    if (CoopModelTestStart(path)) { record("refused_model_test_config", 0, 0); return 15; }
    if (CoopHumanTestStart(path)) { record("refused_human_test_config", 0, 0); return 16; }
    if (CoopTelemetryStart(path)) { record("refused_network_config", 0, 0); return 14; }
    unsigned char patch[5] = {0xe9};
    uint32_t delta = (uint32_t)(uintptr_t)startup_bridge - (0x4f2f05u + 5u);
    memcpy(patch + 1, &delta, sizeof(delta));
    if (!write_startup_patch(patch)) { record("patch_failed", 0, 0); return 13; }
    record("startup_hook_installed", 1, 0);
    return 0;
}
