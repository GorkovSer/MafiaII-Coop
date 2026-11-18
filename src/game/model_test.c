/* Explicit opt-in resource lifecycle test, below observer's complete EXE gate.
   All engine calls run on the game callback thread. No entity is spawned. */
#define WIN32_LEAN_AND_MEAN
#include "model_test.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

int CoopProfileWasRedirected(void);
static wchar_t evidence[MAX_PATH];
static int enabled, attempted, initialized, loaded, paused;

static int readable(const void *pointer, size_t size) {
    MEMORY_BASIC_INFORMATION region;
    if (!pointer || VirtualQuery(pointer, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT || (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    uintptr_t at = (uintptr_t)pointer, end = (uintptr_t)region.BaseAddress + region.RegionSize;
    return end >= at && size <= end - at;
}
static void record(const char *stage, int value) {
    char line[160];
    int n = snprintf(line, sizeof(line), "model_test=%s value=%d thread=%lu\r\n",
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
int CoopModelTestStart(const wchar_t *path) {
    if (!path || wcslen(path) >= MAX_PATH) return 1;
    wcscpy(evidence, path);
    wchar_t ini[MAX_PATH]; wcscpy(ini, path);
    wchar_t *tail = wcsrchr(ini, L'\\'); if (!tail) return 1;
    *tail = 0;
    if (wcslen(ini) + wcslen(L"\\model-test.ini") >= MAX_PATH) return 1;
    wcscat(ini, L"\\model-test.ini");
    enabled = GetPrivateProfileIntW(L"test", L"clone_once", 0, ini) == 1;
    if (!enabled) return 0;
    static const unsigned char factory[] = {0x8b,0x4c,0x24,0x04,0x68,0xa0,0,0,0};
    static const unsigned char clone[] = {0x56,0x57,0x8b,0x7c,0x24,0x0c,0x8b,0xf1};
    static const unsigned char release[] = {0x56,0x8b,0xf1,0x66,0x83,0x7e,0x10,0x01};
    if (!signature(0x14ec940, factory, sizeof(factory)) ||
        !signature(0x14ec8f0, clone, sizeof(clone)) ||
        !signature(0x14e5bc0, release, sizeof(release))) return 2;
    record("enabled", 1); return 0;
}
void CoopModelTestEvent(unsigned int id) {
    if (!enabled || attempted) return;
    if (id == 10 || id == 11 || id == 12 || id == 14 || id == 39) initialized = loaded = 0;
    if (id == 13) initialized = 1;
    if (id == 32) loaded = 0;
    if (id == 33) loaded = 1;
    if (id == 34 || id == 4) paused = 1;
    if (id == 35) paused = 0;
    if (id != 3 || !initialized || !loaded || paused) return;
    if (!CoopProfileWasRedirected()) { attempted = 1; record("refused_profile", 0); return; }
    void *game = *(void **)(uintptr_t)0x1abfe14;
    if (!readable(game, 0x11c)) return;
    void *player = *(void **)((char *)game + 0x118);
    if (!readable(player, 0x64)) return;
    void *source = *(void **)((char *)player + 0x60);
    if (!readable(source, 0xa0)) return;
    /* Limit this first test to the concrete model class audited on disk. */
    if (*(uintptr_t *)source != 0x191c850) {
        attempted = 1; record("refused_source_class", 0); return;
    }
    attempted = 1;
    unsigned short source_refs = *(unsigned short *)((char *)source + 0x10);
    typedef void *(__attribute__((stdcall)) *factory_type)(void *);
    typedef int (__attribute__((thiscall)) *clone_type)(void *, void *);
    typedef unsigned int (__attribute__((thiscall)) *release_type)(void *);
    record("allocating", 1);
    void *model = ((factory_type)(uintptr_t)0x14ec940)(NULL);
    if (!model) { record("allocation_failed", 0); return; }
    if (!readable(model, 0xa0) || *(uintptr_t *)model != 0x191c850 ||
        *(unsigned short *)((char *)model + 0x10) != 1) {
        record("unexpected_allocation_contract", 0); return;
    }
    record("allocated", 1);
    int status = ((clone_type)(uintptr_t)0x14ec8f0)(model, source);
    record("clone_status", status);
    /* Release exactly the single owned reference, on success OR failure.
       Do not dereference model afterwards or append destructor/Free. */
    unsigned int remaining = ((release_type)(uintptr_t)0x14e5bc0)(model);
    model = NULL;
    record("release_remaining", (int)remaining);
    int intact = readable(source, 0x14) && *(uintptr_t *)source == 0x191c850 &&
        *(unsigned short *)((char *)source + 0x10) == source_refs &&
        *(void **)((char *)player + 0x60) == source;
    record("source_reference_intact", intact);
    record("completed", status >= 0 && remaining == 0 && intact);
}
