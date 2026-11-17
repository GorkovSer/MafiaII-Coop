#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include "session.h"
#include <stdio.h>
#include <string.h>

static void record(CoopSession *s, const char *event) { if (s->log) s->log(s->log_context, event); }
static int ipv4(const char *text, enet_uint32 *result) {
    unsigned char bytes[4];
    const char *at = text;
    if (!at) return 0;
    for (unsigned int i = 0; i < 4; ++i) {
        unsigned int n = 0, digits = 0;
        const char *begin=at;
        while (*at >= '0' && *at <= '9') { n = n*10 + (unsigned int)(*at++-'0'); if (++digits > 3 || n > 255) return 0; }
        if (digits>1 && *begin=='0') return 0;
        if (!digits || (i < 3 ? *at++ != '.' : *at != 0)) return 0;
        bytes[i] = (unsigned char)n;
    }
    memcpy(result, bytes, 4);
    return *result != 0xffffffffu;
}
static int send_message(CoopSession *s, CoopMessage *message, unsigned char channel, int reliable) {
    unsigned char bytes[COOP_WIRE_MAX];
    message->session = s->session; message->sequence = ++s->tx_sequence;
    size_t length = CoopWireEncode(message, bytes);
    if (!length || !s->peer) return 0;
    ENetPacket *packet = enet_packet_create(bytes, length, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    if (!packet) return 0;
    if (enet_peer_send(s->peer, channel, packet) < 0) { enet_packet_destroy(packet); return 0; }
    return 1;
}
static void reset_peer(CoopSession *s) {
    s->peer = NULL; s->ready = s->remote_active = s->have_sequence = 0;
    memset(&s->remote, 0, sizeof(s->remote));
    if (s->role == COOP_GUEST) { s->session = 0; s->player_id = 0; }
}
static void reject(CoopSession *s, ENetPeer *peer, const char *reason) {
    ++s->rejected; record(s, reason);
    enet_peer_disconnect_now(peer, 1);
    if (s->peer == peer) reset_peer(s);
}
int CoopSessionStart(CoopSession *s, const CoopSessionConfig *config, CoopSessionLog log, void *context) {
    memset(s, 0, sizeof(*s));
    s->role = config->role; s->config = *config; s->log = log; s->log_context = context;
    ENetAddress address = {0}; address.port = config->port;
    if ((s->role != COOP_HOST && s->role != COOP_GUEST) || !address.port ||
        !ipv4(config->address, &address.host) || (s->role == COOP_GUEST && !address.host)) return 1;
    strcpy(s->address,config->address); s->config.address=s->address;
    if (enet_initialize() < 0) return 2;
    s->host = enet_host_create(s->role == COOP_HOST ? &address : NULL, s->role == COOP_HOST ? 2 : 1, 2, 0, 0);
    if (!s->host) { enet_deinitialize(); return 3; }
    s->host->maximumPacketSize = COOP_WIRE_MAX;
    s->host->maximumWaitingData = 65536;
    s->host->checksum = enet_crc32;
    s->deadline = enet_time_get() + 5000;
    if (s->role == COOP_HOST) {
        if (BCryptGenRandom(NULL, (PUCHAR)&s->session, sizeof(s->session), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0 || !s->session) {
            CoopSessionStop(s); return 4;
        }
        s->player_id = 1;
        record(s, "listening");
    } else {
        s->peer = enet_host_connect(s->host, &address, 2, COOP_PROTOCOL);
        if (!s->peer) { CoopSessionStop(s); return 5; }
        record(s, "connecting");
    }
    return 0;
}
void CoopSessionPublish(CoopSession *s, uint32_t epoch, const float *position) {
    memset(&s->local, 0, sizeof(s->local));
    s->local.type = COOP_STATE; s->local.epoch = epoch; s->local.active = position != NULL;
    if (position) memcpy(s->local.position, position, sizeof(s->local.position));
}
static void receive(CoopSession *s, ENetEvent *event, uint32_t now) {
    CoopMessage message;
    if (!CoopWireDecode(event->packet->data, event->packet->dataLength, &message)) {
        reject(s, event->peer, "reject_wire"); return;
    }
    if (event->peer != s->peer) { reject(s, event->peer, "reject_peer"); return; }
    if (message.type == COOP_HELLO && s->role == COOP_HOST && !s->ready && event->channelID == 0) {
        if (message.session || message.capabilities != s->config.capabilities ||
            message.module_version!=s->config.module_version || memcmp(message.build, s->config.build, 32)) {
            reject(s, event->peer, "reject_compatibility"); return;
        }
        CoopMessage welcome = {0}; welcome.type = COOP_WELCOME; welcome.player_id = 2;
        welcome.capabilities = s->config.capabilities;
        welcome.module_version=s->config.module_version;
        send_message(s, &welcome, 0, 1);
        s->ready = 1; s->next_send = now;
        ++s->connections; record(s, "ready_host");
    } else if (message.type == COOP_WELCOME && s->role == COOP_GUEST && !s->ready && event->channelID == 0) {
        if (!message.session || message.player_id != 2 || message.capabilities != s->config.capabilities ||
            message.module_version!=s->config.module_version) {
            reject(s, event->peer, "reject_welcome"); return;
        }
        s->session = message.session; s->player_id = message.player_id;
        s->ready = 1; s->next_send = now;
        ++s->connections; record(s, "ready_guest");
    } else if (message.type == COOP_STATE && s->ready && event->channelID == 1) {
        uint32_t remote_id = s->role == COOP_HOST ? 2 : 1;
        if (message.session != s->session || message.player_id != remote_id) {
            reject(s, event->peer, "reject_identity"); return;
        }
        if (s->have_sequence && !CoopSequenceNewer(message.sequence, s->rx_sequence)) { ++s->stale; return; }
        if (s->have_sequence && message.epoch != s->remote.epoch && !CoopSequenceNewer(message.epoch, s->remote.epoch)) {
            ++s->stale; return;
        }
        s->remote = message; s->rx_sequence = message.sequence; s->have_sequence = 1;
        if (s->remote_active != (int)message.active) record(s, message.active ? "remote_active" : "remote_inactive");
        s->remote_active = (int)message.active; ++s->received_states;
    } else { reject(s, event->peer, "reject_phase"); return; }
    s->deadline = now + 10000;
}
void CoopSessionTick(CoopSession *s) {
    if (!s->host) return;
    uint32_t now = enet_time_get();
    ENetEvent event;
    for (unsigned int count = 0; count < 32 && enet_host_service(s->host, &event, 0) > 0; ++count) {
        if (event.type == ENET_EVENT_TYPE_CONNECT) {
            if (s->role == COOP_HOST) {
                if (s->peer || event.data != COOP_PROTOCOL) { reject(s, event.peer, "reject_busy_or_protocol"); continue; }
                s->peer = event.peer;
            }
            enet_peer_timeout(event.peer, 8, 3000, 10000);
            s->deadline = now + 5000;
            if (s->role == COOP_GUEST) {
                s->guest_connected = 1;
                CoopMessage hello = {0}; hello.type = COOP_HELLO;
                memcpy(hello.build, s->config.build, 32); hello.capabilities = s->config.capabilities;
                hello.module_version=s->config.module_version;
                send_message(s, &hello, 0, 1);
            }
        } else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            receive(s, &event, now); enet_packet_destroy(event.packet);
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT && event.peer == s->peer) {
            reset_peer(s); record(s, "disconnected");
        }
    }
    if (s->peer && (int32_t)(now - s->deadline) >= 0) {
        enet_peer_disconnect_now(s->peer, 2); reset_peer(s); record(s, "timeout");
    }
    if (s->ready && (int32_t)(now - s->next_send) >= 0) {
        CoopMessage state = s->local; state.type = COOP_STATE; state.player_id = s->player_id;
        if (send_message(s, &state, 1, 0)) ++s->sent_states;
        s->next_send = now + (state.active ? 50 : 500);
        enet_host_flush(s->host);
    }
}
void CoopSessionStop(CoopSession *s) {
    if (!s->host) return;
    if (s->peer) enet_peer_disconnect_now(s->peer, 0);
    reset_peer(s); enet_host_destroy(s->host); s->host = NULL;
    enet_deinitialize(); record(s, "stopped");
}
