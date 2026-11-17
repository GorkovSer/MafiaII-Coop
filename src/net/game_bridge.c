#define WIN32_LEAN_AND_MEAN
#include "game_bridge.h"
#include "session.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static CoopSession session;
static wchar_t evidence[MAX_PATH];
static HANDLE worker, stop_event;
static SRWLOCK mailbox_lock = SRWLOCK_INIT;
static struct {uint32_t epoch; int active; float xyz[3]; ULONGLONG published;} latest;
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
    log_event(NULL, "worker_started");
    while (WaitForSingleObject(stop_event, 5) == WAIT_TIMEOUT) {
        AcquireSRWLockExclusive(&mailbox_lock);
        uint32_t epoch = latest.epoch;
        int active = latest.active && GetTickCount64()-latest.published < 2000;
        float xyz[3]; memcpy(xyz, latest.xyz, sizeof(xyz));
        ReleaseSRWLockExclusive(&mailbox_lock);
        CoopSessionPublish(&session, epoch, active ? xyz : NULL);
        CoopSessionTick(&session);
        AcquireSRWLockExclusive(&mailbox_lock);
        remote.ready = (uint32_t)session.ready;
        remote.player_id = session.remote.player_id; remote.epoch = session.remote.epoch;
        remote.active = (uint32_t)session.remote_active;
        remote.rtt_ms = session.peer ? session.peer->roundTripTime : 0;
        remote.received_states = session.received_states;
        memcpy(remote.position, session.remote.position, sizeof(remote.position));
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
    stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!stop_event) {CoopSessionStop(&session); return 6;}
    worker = CreateThread(NULL, 0, network_worker, NULL, 0, NULL);
    if (!worker) {CloseHandle(stop_event); stop_event=NULL; CoopSessionStop(&session); return 7;}
    return 0;
}
int CoopNetworkReadRemote(CoopRemoteSnapshot *out) {
    if (!worker || !out) return 0;
    AcquireSRWLockShared(&mailbox_lock); *out=remote; ReleaseSRWLockShared(&mailbox_lock);
    return out->ready != 0;
}
void CoopNetworkPublish(uint32_t epoch, const float *position) {
    if (!worker) return;
    AcquireSRWLockExclusive(&mailbox_lock);
    latest.epoch=epoch; latest.active=position != NULL; latest.published=GetTickCount64();
    if(position) memcpy(latest.xyz,position,sizeof(latest.xyz)); else memset(latest.xyz,0,sizeof(latest.xyz));
    ReleaseSRWLockExclusive(&mailbox_lock);
}
void CoopNetworkStop(void) {
    if (!worker) return;
    SetEvent(stop_event);
    /* No thread termination or DLL unload on a delayed stop. The process
       retains the worker and its handles until it exits. */
    if (WaitForSingleObject(worker,1000) != WAIT_OBJECT_0) return;
    CloseHandle(worker); CloseHandle(stop_event); worker=stop_event=NULL;
}
