#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "game_bridge.h"
#include "session.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static CoopSession session;
static wchar_t evidence[MAX_PATH];
static HANDLE worker, stop_event;
static SRWLOCK mailbox_lock = SRWLOCK_INIT;
static struct {uint32_t epoch; int active; float xyz[3], heading;
    uint32_t heading_valid; ULONGLONG published;} latest;
static struct {uint32_t epoch; unsigned char key[32]; ULONGLONG published, revision;} loaded_world;
static CoopRemoteSnapshot remote;
static void log_event(void *context, const char *event) {
    (void)context;
    char line[192];
    int size = snprintf(line, sizeof(line), "network=%s thread=%lu sent=%lu received=%lu\r\n", event,
        (unsigned long)GetCurrentThreadId(), (unsigned long)session.sent_states, (unsigned long)session.received_states);
    if (size <= 0 || (size_t)size >= sizeof(line)) return;
    HANDLE file = CreateFileW(evidence, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(file, line, (DWORD)size, &written, NULL); CloseHandle(file);
}
static DWORD WINAPI network_worker(void *context) {
    (void)context;
    uint64_t applied_world_revision = 0;
    log_event(NULL, "worker_started");
    while (WaitForSingleObject(stop_event, 5) == WAIT_TIMEOUT) {
        AcquireSRWLockExclusive(&mailbox_lock);
        uint32_t epoch = latest.epoch;
        int active = latest.active && GetTickCount64()-latest.published < COOP_STATE_MAX_AGE_MS;
        float xyz[3]; memcpy(xyz, latest.xyz, sizeof(xyz));
        float heading = latest.heading;
        uint32_t heading_valid = latest.heading_valid;
        uint32_t world_epoch = loaded_world.epoch;
        unsigned char world_key[32]; memcpy(world_key, loaded_world.key, 32);
        uint64_t world_observed = loaded_world.published;
        uint64_t world_revision = loaded_world.revision;
        ReleaseSRWLockExclusive(&mailbox_lock);
        uint64_t before = GetTickCount64();
        int world_fresh = before >= world_observed && before - world_observed < COOP_STATE_MAX_AGE_MS;
        if (world_revision != applied_world_revision) {
            /* Do not lose a withdrawal/replacement coalesced between polls. */
            CoopSessionWorld(&session, 0, NULL);
            applied_world_revision = world_revision;
        }
        CoopSessionWorld(&session, world_fresh ? world_epoch : 0, world_fresh ? world_key : NULL);
        CoopSessionPublishPose(&session, epoch, active ? xyz : NULL,
            active && heading_valid ? &heading : NULL);
        CoopSessionTick(&session);
        uint64_t published = GetTickCount64();
        uint32_t age = CoopSessionRemoteAge(&session, enet_time_get());
        AcquireSRWLockExclusive(&mailbox_lock);
        remote.ready = (uint32_t)session.ready;
        remote.player_id = session.remote.player_id; remote.epoch = session.remote.epoch;
        remote.active = (uint32_t)session.remote_active;
        remote.rtt_ms = session.peer ? session.peer->roundTripTime : 0;
        remote.received_states = session.received_states;
        remote.have_state = (uint32_t)(session.have_sequence && age <= published);
        remote.received_sequence = session.rx_sequence;
        remote.connection_generation = session.connections;
        remote.session_id = session.session;
        remote.state_received_ms = remote.have_state ? published - age : 0;
        remote.mailbox_published_ms = published;
        remote.world_authorized = (uint32_t)(world_revision == loaded_world.revision &&
            active && epoch == world_epoch && world_fresh &&
            CoopWorldAccepts(&session.world, session.remote.world.generation, session.remote.epoch));
        remote.world_generation = session.remote.world.generation;
        remote.world_observed_ms = world_observed;
        memcpy(remote.position, session.remote.position, sizeof(remote.position));
        remote.heading = session.remote.heading; remote.heading_valid = session.remote.heading_valid;
        ReleaseSRWLockExclusive(&mailbox_lock);
    }
    CoopSessionStop(&session);
    return 0;
}
int CoopNetworkStart(const wchar_t *log_path) {
    if (!log_path || wcslen(log_path) >= MAX_PATH) return 1;
    wcscpy(evidence, log_path);
    wchar_t ini[MAX_PATH]; wcscpy(ini, log_path);
    wchar_t *tail = wcsrchr(ini, L'\\');
    if (!tail) return 1;
    *tail = 0;
    if (wcslen(ini) + wcslen(L"\\coop-session.ini") >= MAX_PATH) return 1;
    wcscat(ini, L"\\coop-session.ini");
    if (GetFileAttributesW(ini) == INVALID_FILE_ATTRIBUTES) return 0; /* Explicit opt-in. */
    wchar_t role[16], address[64];
    GetPrivateProfileStringW(L"network", L"role", L"", role, 16, ini);
    GetPrivateProfileStringW(L"network", L"address", L"", address, 64, ini);
    UINT port = GetPrivateProfileIntW(L"network", L"port", 0, ini);
    char ascii_address[64];
    unsigned int i = 0;
    for (; address[i] && i < sizeof(ascii_address)-1; ++i) {
        if (address[i] > 127) return 2;
        ascii_address[i] = (char)address[i];
    }
    ascii_address[i] = 0;
    CoopSessionConfig config = {0};
    config.role = !wcscmp(role, L"host") ? COOP_HOST : !wcscmp(role, L"guest") ? COOP_GUEST : 0;
    if (!port || port > 65535) return 2;
    config.address = ascii_address; config.port = (uint16_t)port; config.capabilities = COOP_CAP_TELEMETRY;
    config.module_version=COOP_MODULE_VERSION;
    memcpy(config.build, CoopGameBuild, 32);
    int result = CoopSessionStart(&session, &config, log_event, NULL);
    if (result) {log_event(NULL, "start_failed"); return result;}
    memset(&latest, 0, sizeof(latest)); memset(&remote, 0, sizeof(remote));
    memset(&loaded_world, 0, sizeof(loaded_world));
    stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!stop_event) {CoopSessionStop(&session); return 6;}
    worker = CreateThread(NULL, 0, network_worker, NULL, 0, NULL);
    if (!worker) {CloseHandle(stop_event); stop_event=NULL; CoopSessionStop(&session); return 7;}
    return 0;
}
int CoopNetworkReadRemote(CoopRemoteSnapshot *out) {
    if (!out) return 0;
    memset(out, 0, sizeof(*out));
    if (!worker || !stop_event || WaitForSingleObject(stop_event, 0) != WAIT_TIMEOUT) return 0;
    CoopRemoteSnapshot snapshot;
    AcquireSRWLockShared(&mailbox_lock); snapshot=remote; ReleaseSRWLockShared(&mailbox_lock);
    return CoopRemoteSnapshotAt(&snapshot, GetTickCount64(), out);
}
void CoopNetworkPublish(uint32_t epoch, const float *position) {
    CoopNetworkPublishPose(epoch, position, NULL);
}
void CoopNetworkPublishPose(uint32_t epoch, const float *position, const float *heading) {
    if (!worker) return;
    AcquireSRWLockExclusive(&mailbox_lock);
    latest.epoch=epoch; latest.active=position != NULL; latest.published=GetTickCount64();
    if(position) memcpy(latest.xyz,position,sizeof(latest.xyz)); else memset(latest.xyz,0,sizeof(latest.xyz));
    latest.heading_valid = position && heading && CoopHeadingValid(*heading, 1);
    latest.heading = latest.heading_valid ? *heading : 0;
    ReleaseSRWLockExclusive(&mailbox_lock);
}
void CoopNetworkPublishWorld(uint32_t epoch, const unsigned char *key) {
    if (!worker) return;
    unsigned char empty[32] = {0};
    if (!epoch || !CoopWorldKeyValid(key)) { epoch = 0; key = empty; }
    AcquireSRWLockExclusive(&mailbox_lock);
    if (epoch != loaded_world.epoch || memcmp(key, loaded_world.key, 32)) {
        remote.world_authorized = 0; ++loaded_world.revision;
    }
    loaded_world.epoch = epoch; memcpy(loaded_world.key, key, 32);
    loaded_world.published = GetTickCount64();
    ReleaseSRWLockExclusive(&mailbox_lock);
}
int CoopNetworkReadRemoteWorld(CoopRemoteSnapshot *out) {
    if (!out) return 0;
    if (CoopNetworkReadRemote(out) && out->active && out->world_authorized) {
        uint64_t now = GetTickCount64();
        AcquireSRWLockShared(&mailbox_lock);
        int local = remote.world_authorized && remote.world_generation == out->world_generation &&
            remote.session_id == out->session_id && remote.connection_generation == out->connection_generation &&
            remote.epoch == out->epoch && latest.active && latest.epoch == loaded_world.epoch &&
            now >= latest.published && now - latest.published < COOP_STATE_MAX_AGE_MS &&
            now >= loaded_world.published && now - loaded_world.published < COOP_STATE_MAX_AGE_MS;
        ReleaseSRWLockShared(&mailbox_lock);
        if (local) return 1;
    }
    memset(out, 0, sizeof(*out)); return 0;
}
void CoopNetworkStop(void) {
    if (!worker) return;
    SetEvent(stop_event);
    /* No thread termination or DLL unload on a delayed stop. The process
       retains the worker and its handles until it exits. */
    if (WaitForSingleObject(worker,1000) != WAIT_OBJECT_0) return;
    CloseHandle(worker); CloseHandle(stop_event); worker=stop_event=NULL;
}
