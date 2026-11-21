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
    uint32_t world_authorized, world_generation;
    uint64_t world_observed_ms;
} CoopRemoteSnapshot;
/* Pure receiver-local validity filter. Does not read a worker or call the game. */
int CoopRemoteSnapshotAt(const CoopRemoteSnapshot *source, uint64_t now, CoopRemoteSnapshot *out);
int CoopNetworkReadRemote(CoopRemoteSnapshot *out);
/* Actor application must use this stricter gate, not telemetry readiness. */
int CoopNetworkReadRemoteWorld(CoopRemoteSnapshot *out);
/* loaded_epoch must change for every actual load/rewind, even for the same
   digest. NULL immediately withdraws local loaded-world authority. No producer is
   enabled until a verified game-specific world identity is implemented. */
void CoopNetworkPublishWorld(uint32_t loaded_epoch, const unsigned char *verified_key);
void CoopNetworkPublish(uint32_t epoch, const float *position);
void CoopNetworkStop(void);
#endif
