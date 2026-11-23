#ifndef COOP_REMOTE_TIMELINE_H
#define COOP_REMOTE_TIMELINE_H
#include "game_bridge.h"

#define COOP_TIMELINE_CAPACITY 8u
#define COOP_TIMELINE_DELAY_MS 100u
#define COOP_TIMELINE_MAX_GAP_MS 250u

typedef struct {
    CoopRemoteSnapshot frames[COOP_TIMELINE_CAPACITY];
    unsigned int count;
    uint64_t last_now;
} CoopRemoteTimeline;
typedef struct {
    CoopRemoteSnapshot authority; /* Fresh current authority, never synthesized. */
    float position[3];            /* Receiver-local render/movement target. */
    unsigned int interpolated;
} CoopRemoteFrame;

/* Game-thread only; zero initialize once. Every call must supply the CURRENT
   CoopNetworkReadRemoteWorld result, including NULL on withdrawal. No cached
   authority, extrapolation, game calls, teleports or packet generation. */
int CoopRemoteTimelineAt(CoopRemoteTimeline *timeline,
    const CoopRemoteSnapshot *current, uint64_t now_ms, CoopRemoteFrame *out);
#endif
