/* Read-only state adapter for the SHA256-gated GOG build. See RESEARCH.md.
   Called only on the registered game thread. No engine function calls. */
#define WIN32_LEAN_AND_MEAN
#include "telemetry.h"
#include "../net/game_bridge.h"
#include "story_test.h"
#include "script_progress.h"
#include "player_control.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static wchar_t evidence_path[MAX_PATH];
static uint32_t epoch;
static int initialized, loaded, paused, pause_known, last_status = -1;
static ULONGLONG next_sample, next_log, next_progress_log;

/* Only the standalone fixture build substitutes its own pointer storage and
   clock. The game DLL retains the audited singleton and Windows clock. */
#ifdef COOP_TELEMETRY_TEST
extern uintptr_t CoopTelemetryTestGameSlot(void);
extern ULONGLONG CoopTelemetryTestNow(void);
#define GAME_SLOT CoopTelemetryTestGameSlot()
#define SAMPLE_TIME() CoopTelemetryTestNow()
/* Standalone telemetry fixture never reads the game's script singleton. */
#define SCRIPT_SLOT 0u
#else
#define GAME_SLOT ((uintptr_t)0x1abfe14)
#define SAMPLE_TIME() GetTickCount64()
#define SCRIPT_SLOT 0x1abfe24u
#endif

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
    if (!pointer_at(GAME_SLOT, &game) || !pointer_at(game + 0x118, &player)) return 1;
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
        /* Visual model basis, matching the pinned frame GetDirection path.
           A missing/degenerate basis leaves facing unknown; never invent it
           from velocity, since a standing player may turn in place. */
        uintptr_t model_table;
        float x, y;
        if (pointer_at(model, &model_table) && model_table == 0x191c850 &&
            copy_at(model + 0x3c, &x, 4) && copy_at(model + 0x4c, &y, 4) &&
            isfinite(x) && isfinite(y) && fabsf(x) <= 100 && fabsf(y) <= 100 &&
            x*x+y*y > 0.000001f) {
            out->heading = atan2f(y, x); out->heading_valid = 1;
        }
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

static int progress_read(void *context, uint32_t address, void *out, size_t size) {
    (void)context;
    return copy_at(address, out, size);
}

static void log_player_control(uint32_t guid) {
    uintptr_t game, player, game_again, player_again;
    CoopPlayerControl control = {0};
    int status = COOP_PLAYER_CONTROL_UNREADABLE;
    if (pointer_at(GAME_SLOT, &game) && pointer_at(game + 0x118, &player)) {
        status = CoopPlayerControlObserve(progress_read, NULL, (uint32_t)player, guid, &control);
        if (!pointer_at(GAME_SLOT, &game_again) || game_again != game ||
            !pointer_at(game_again + 0x118, &player_again) || player_again != player) {
            status = COOP_PLAYER_CONTROL_CHANGED;
            memset(&control, 0, sizeof(control));
        }
    }
    char line[160];
    int size = snprintf(line, sizeof(line),
        "player_control status=%d mode=%lu native_lock_query=%d ownership_verified=0\r\n",
        status, (unsigned long)control.mode, control.native_lock_query);
    append(line, size);
}

static void log_script_progress(unsigned int event_id) {
    int transition = event_id == 10 || event_id == 11 || event_id == 12 || event_id == 13 ||
        event_id == 14 || event_id == 19 || event_id == 20 || event_id == 21 ||
        event_id == 27 || event_id == 32 || event_id == 33 || event_id == 39;
    if (!evidence_path[0] || (!transition && event_id != 3 && event_id != 4 && event_id != 22)) return;
    ULONGLONG now = SAMPLE_TIME();
    if (!transition && now < next_progress_log) return;
    next_progress_log = now + 500;
    CoopScriptProgress progress;
    CoopProgressStatus status = CoopReadScriptProgress(progress_read, NULL, SCRIPT_SLOT, &progress);
    char line[512];
    int size = snprintf(line, sizeof(line),
        "script_progress ms=%llu event=%u epoch=%lu initialized=%d loaded=%d paused=%d status=%d world_verified=0 checkpoint_verified=0",
        (unsigned long long)now, event_id, (unsigned long)epoch, initialized, loaded, paused, (int)status);
    if (size <= 0 || (size_t)size >= sizeof(line)) return;
    int tail;
    if (status == COOP_PROGRESS_OBSERVED)
        tail = snprintf(line + size, sizeof(line) - (size_t)size, " mission_number=%lu mission_part=%lu\r\n",
            (unsigned long)progress.mission_number, (unsigned long)progress.mission_part);
    else tail = snprintf(line + size, sizeof(line) - (size_t)size, "\r\n");
    if (tail <= 0 || (size_t)tail >= sizeof(line) - (size_t)size) return;
    append(line, size + tail);
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
    if (event_id == 34) {paused = 1; pause_known = 1;}
    if (event_id == 35) {paused = 0; pause_known = 1;}
    /* A final paused-frame callback can arrive AFTER explicit unpause. Do not
       let that stale frame relatch pause; lifecycle events own the known state.
       Before any lifecycle observation, paused frames still fail closed. */
    if (event_id == 4 && !pause_known) paused = 1;
    if (event_id == 11 || event_id == 12 || event_id == 14 || event_id == 39) initialized = loaded = 0;
    /* A pause can stop ordinary ticks completely. Invalidate the published
       position at the pause callback instead of retaining it for the worker's
       two-second producer-stall timeout. Only an explicit unpause allows
       subsequent loaded ticks to republish. */
    if (!initialized || !loaded || paused)
        CoopNetworkPublish(epoch, NULL);
    log_script_progress(event_id);
    if (event_id == 39) CoopNetworkStop();
    if (event_id != 3 || !initialized || !loaded || paused || !evidence_path[0]) return;
    ULONGLONG now = SAMPLE_TIME();
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
    CoopNetworkPublishPose(epoch, snapshot.position, snapshot.heading_valid ? &snapshot.heading : NULL);
    if (now < next_log) return;
    next_log = now + 500;
    log_player_control(snapshot.local_guid);
    int size = snprintf(line, sizeof(line),
        "player_sample ms=%llu epoch=%lu guid=%lu xyz=%.5f,%.5f,%.5f model_valid=%d model=%.5f,%.5f,%.5f heading_valid=%d heading=%.6f\r\n",
        (unsigned long long)now, (unsigned long)snapshot.epoch, (unsigned long)snapshot.local_guid,
        (double)snapshot.position[0], (double)snapshot.position[1], (double)snapshot.position[2],
        snapshot.model_valid, (double)snapshot.model_position[0], (double)snapshot.model_position[1],
        (double)snapshot.model_position[2], snapshot.heading_valid, (double)snapshot.heading);
    append(line, size);
}
