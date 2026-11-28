#ifndef COOP_REMOTE_TIMELINE_H
#define COOP_REMOTE_TIMELINE_H
#include "game_bridge.h"

#define COOP_TIMELINE_CAPACITY 8u
#define COOP_TIMELINE_DELAY_MS 100u
#define COOP_TIMELINE_MAX_GAP_MS 250u
/* Discontinuities are new anchors, not a path to interpolate through. These
   conservative world-unit limits match the existing actor distance policy;
   they are not speed estimates or permission to teleport a physical actor. */
#define COOP_TIMELINE_MAX_STEP_XY 20.0f
#define COOP_TIMELINE_MAX_STEP_Z 3.0f
#define COOP_TIMELINE_RENDER_HEADING 1

typedef struct {
    CoopRemoteSnapshot frames[COOP_TIMELINE_CAPACITY];
    unsigned int count;
    uint64_t last_now;
    /* Preserve packet identity when refusing rollback/mutation. Otherwise the
       same rejected mailbox could seed an empty history on the next tick. */
    CoopRemoteSnapshot newest;
    unsigned int have_newest;
} CoopRemoteTimeline;
typedef struct {
    CoopRemoteSnapshot authority; /* Fresh current authority, never synthesized. */
    float position[3];            /* Receiver-local render/movement target. */
    unsigned int interpolated;
    float heading;               /* Delayed shortest-arc presentation angle. */
    unsigned int heading_valid;  /* No invented angle across an unknown pose. */
} CoopRemoteFrame;

/* Game-thread only; zero initialize once. Every call must supply the CURRENT
   CoopNetworkReadRemoteWorld result, including NULL on withdrawal. No cached
   authority, extrapolation, game calls, teleports or packet generation.
   Render position AND heading from this frame; authority always remains the
   exact current snapshot and must not be replaced with the delayed pose.
   No interpolation across large position jumps; the new pose is the anchor. */
int CoopRemoteTimelineAt(CoopRemoteTimeline *timeline,
    const CoopRemoteSnapshot *current, uint64_t now_ms, CoopRemoteFrame *out);
#endif
