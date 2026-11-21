#ifndef COOP_SESSION_H
#define COOP_SESSION_H
#include "wire.h"
#include <enet/enet.h>
enum { COOP_HOST = 1, COOP_GUEST = 2 };
typedef struct {
    int role;
    const char *address;
    uint16_t port;
    uint32_t capabilities;
    uint32_t module_version;
    unsigned char build[32];
} CoopSessionConfig;
typedef void (*CoopSessionLog)(void *, const char *);
typedef struct {
    ENetHost *host;
    ENetPeer *peer;
    int role, ready, remote_active, have_sequence, guest_connected;
    uint64_t session;
    uint32_t player_id, tx_sequence, rx_sequence, deadline, next_send;
    uint32_t sent_states, received_states, rejected, stale, connections;
    uint32_t remote_received_at;
    CoopMessage local, remote;
    CoopWorldSync world;
    CoopSessionConfig config;
    char address[16];
    CoopSessionLog log;
    void *log_context;
} CoopSession;
int CoopSessionStart(CoopSession *session, const CoopSessionConfig *config, CoopSessionLog log, void *context);
void CoopSessionPublish(CoopSession *session, uint32_t epoch, const float *position);
void CoopSessionWorld(CoopSession *session, uint32_t loaded_epoch, const unsigned char *verified_key);
void CoopSessionTick(CoopSession *session);
uint32_t CoopSessionRemoteAge(const CoopSession *session, uint32_t now);
void CoopSessionStop(CoopSession *session);
#endif
