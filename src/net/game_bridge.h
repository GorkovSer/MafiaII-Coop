#ifndef COOP_GAME_BRIDGE_H
#define COOP_GAME_BRIDGE_H
#include <stddef.h>
#include <stdint.h>
int CoopNetworkStart(const wchar_t *log_path);
typedef struct {
    uint32_t ready, player_id, epoch, active, rtt_ms, received_states;
    float position[3];
    uint32_t have_state, received_sequence, connection_generation;
    uint64_t session_id, state_received_ms, mailbox_published_ms;
} CoopRemoteSnapshot;
/* Pure receiver-local validity filter. Does not read a worker or call the game. */
int CoopRemoteSnapshotAt(const CoopRemoteSnapshot *source, uint64_t now, CoopRemoteSnapshot *out);
int CoopNetworkReadRemote(CoopRemoteSnapshot *out);
void CoopNetworkPublish(uint32_t epoch, const float *position);
void CoopNetworkStop(void);
#endif
