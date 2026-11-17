#ifndef COOP_GAME_BRIDGE_H
#define COOP_GAME_BRIDGE_H
#include <windows.h>
#include <stdint.h>
int CoopNetworkStart(const wchar_t *log_path);
typedef struct {
    uint32_t ready, player_id, epoch, active, rtt_ms, received_states;
    float position[3];
} CoopRemoteSnapshot;
int CoopNetworkReadRemote(CoopRemoteSnapshot *out);
void CoopNetworkPublish(uint32_t epoch, const float *position);
void CoopNetworkStop(void);
#endif
