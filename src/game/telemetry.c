/* Read-only state adapter for the SHA256-gated GOG build. See RESEARCH.md.
   Called only on the registered game thread. No engine function calls. */
#define WIN32_LEAN_AND_MEAN
#include "telemetry.h"
#include "../net/game_bridge.h"
#include "story_test.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static wchar_t evidence_path[MAX_PATH];
static uint32_t epoch;
static int initialized, loaded, last_status = -1;
static ULONGLONG next_sample, next_log;

static int copy_at(uintptr_t at, void *out, size_t size) {
    MEMORY_BASIC_INFORMATION region;
    if (!at || VirtualQuery((void *)at, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT || (region.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return 0;
    uintptr_t end = (uintptr_t)region.BaseAddress + region.RegionSize;
    if (end < at || size > end - at) return 0;
    memcpy(out, (const void *)at, size);
    return 1;
}

static int pointer_at(uintptr_t at, uintptr_t *out) {
    return copy_at(at, out, sizeof(*out)) && *out != 0;
}

static int finite_position(const float xyz[3]) {
    for (unsigned int i = 0; i < 3; ++i)
        if (!isfinite(xyz[i]) || fabsf(xyz[i]) >= 1000000.0f) return 0;
    return 1;
}

/* The observed virtual GetPosition is 0x938250. Its physics path invokes
   vtable+0x54, whose audited implementation 0x18ae690 returns this+0x1d8.
   Read that storage directly; do not invoke a candidate virtual function. */
static int read_player(CoopPlayerSnapshot *out) {
    uintptr_t game, player, vtable, target, physics, physics_vtable, getter, model;
    memset(out, 0, sizeof(*out));
    out->epoch = epoch;
    if (!pointer_at(0x1abfe14, &game) || !pointer_at(game + 0x118, &player)) return 1;
    if (!pointer_at(player, &vtable) || !pointer_at(vtable + 0x8c, &target) ||
        target != 0x938250) return 2;
    if (!pointer_at(player + 0x190, &physics) || !pointer_at(physics, &physics_vtable) ||
        !pointer_at(physics_vtable + 0x54, &getter) || getter != 0x18ae690) return 3;
    if (!copy_at(physics + 0x1d8, out->position, sizeof(out->position)) ||
        !finite_position(out->position) || !copy_at(player + 0x1c, &out->local_guid, 4)) return 4;
    if (pointer_at(player + 0x60, &model)) {
        out->model_valid = copy_at(model + 0x44, &out->model_position[0], 4) &&
            copy_at(model + 0x54, &out->model_position[1], 4) &&
            copy_at(model + 0x64, &out->model_position[2], 4) && finite_position(out->model_position);
    }
    return 0;
}

static void append(const char *line, int size) {
    if (size <= 0 || size >= 512) return;
    HANDLE file = CreateFileW(evidence_path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(file, line, (DWORD)size, &written, NULL);
    CloseHandle(file);
}

int CoopTelemetryStart(const wchar_t *path) {
    if (path && wcslen(path) < MAX_PATH) wcscpy(evidence_path, path);
    if(CoopStoryTestStart(path))return 1;
    return CoopNetworkStart(path);
}

void CoopTelemetryEvent(unsigned int event_id) {
    CoopStoryTestEvent(event_id);
    if (event_id == 10) { ++epoch; initialized = loaded = 0; last_status = -1; next_sample = next_log = 0; }
    if (event_id == 13) initialized = 1;
    if (event_id == 32) loaded = 0;
    if (event_id == 33) loaded = 1;
    if (event_id == 11 || event_id == 12 || event_id == 14 || event_id == 39) initialized = loaded = 0;
    if (!initialized || !loaded) CoopNetworkPublish(epoch, NULL);
    if (event_id == 39) CoopNetworkStop();
    if (event_id != 3 || !initialized || !loaded || !evidence_path[0]) return;
    ULONGLONG now = GetTickCount64();
    if (now < next_sample) return;
    next_sample = now + 50;
    CoopPlayerSnapshot snapshot;
    int status = read_player(&snapshot);
    char line[512];
    if (status != last_status) {
        int size = snprintf(line, sizeof(line), "player_reader_status=%d epoch=%lu thread=%lu\r\n",
            status, (unsigned long)epoch, (unsigned long)GetCurrentThreadId());
        append(line, size);
        last_status = status;
    }
    if (status) { CoopNetworkPublish(epoch, NULL); return; }
    CoopNetworkPublish(epoch, snapshot.position);
    if (now < next_log) return;
    next_log = now + 500;
    int size = snprintf(line, sizeof(line),
        "player_sample ms=%llu epoch=%lu guid=%lu xyz=%.5f,%.5f,%.5f model_valid=%d model=%.5f,%.5f,%.5f\r\n",
        (unsigned long long)now, (unsigned long)snapshot.epoch, (unsigned long)snapshot.local_guid,
        (double)snapshot.position[0], (double)snapshot.position[1], (double)snapshot.position[2],
        snapshot.model_valid, (double)snapshot.model_position[0], (double)snapshot.model_position[1],
        (double)snapshot.model_position[2]);
    append(line, size);
}
