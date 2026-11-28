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
#define COOP_TIMELINE_INTERVAL_CAPACITY 16u
#define COOP_TIMELINE_DELAY_LIMIT_MS 200u

typedef enum {
    COOP_INTERPOLATION_FIXED=0,
    COOP_INTERPOLATION_ADAPTIVE=1
} CoopInterpolationMode;
typedef struct {
    unsigned int mode;
    unsigned int fixed_delay_ms;  /* 0..200; default 100. */
    unsigned int min_delay_ms;    /* 50..max_delay_ms. */
    unsigned int max_delay_ms;    /* min_delay_ms..200. */
    unsigned int warmup_intervals; /* 4..16 positive receive intervals. */
} CoopTimelineConfig;

typedef enum {
    COOP_TIMELINE_RESET_NONE=0, COOP_TIMELINE_RESET_EXPLICIT,
    COOP_TIMELINE_RESET_CONFIG, COOP_TIMELINE_RESET_SCOPE,
    COOP_TIMELINE_RESET_WITHDRAWAL, COOP_TIMELINE_RESET_INVALID,
    COOP_TIMELINE_RESET_EXPIRED, COOP_TIMELINE_RESET_GAP,
    COOP_TIMELINE_RESET_POSITION, COOP_TIMELINE_RESET_CLOCK,
    COOP_TIMELINE_RESET_REJECTED, COOP_TIMELINE_RESET_CORRUPT,
    COOP_TIMELINE_RESET_REASON_COUNT
} CoopTimelineResetReason;
typedef enum {
    COOP_TIMELINE_REFUSAL_NONE=0, COOP_TIMELINE_REFUSAL_WITHDRAWAL,
    COOP_TIMELINE_REFUSAL_INVALID, COOP_TIMELINE_REFUSAL_EXPIRED,
    COOP_TIMELINE_REFUSAL_SEQUENCE, COOP_TIMELINE_REFUSAL_TIMESTAMP,
    COOP_TIMELINE_REFUSAL_DUPLICATE, COOP_TIMELINE_REFUSAL_CLOCK,
    COOP_TIMELINE_REFUSAL_REASON_COUNT
} CoopTimelineRefusalReason;
typedef struct {
    /* Scope-local counters; never RTT, one-way latency or loss measurements. */
    uint64_t accepted_snapshots, positive_intervals, same_ms_bursts, long_gaps;
    uint64_t underrun_events, history_evictions, latency_clamps;
    unsigned int window_intervals, mean_interval_us, variation_us;
    unsigned int last_interval_ms, history_peak;
} CoopTimelineDeliveryStats;
typedef struct {
    unsigned int mode, delay_ms, effective_delay_ms, snapshot_count;
    unsigned int warming, holding, presentation_valid;
    uint64_t presentation_ms;
    CoopTimelineDeliveryStats delivery;
    /* Lifetime counters saturate, survive stream resets; zero-init clears all.
       Refusals count calls; resets count actual transitions (explicit/config
       resets always count). Reasons use the enums above, no external logger. */
    uint64_t resets[COOP_TIMELINE_RESET_REASON_COUNT];
    uint64_t refusals[COOP_TIMELINE_REFUSAL_REASON_COUNT];
    unsigned int last_reset_reason, last_refusal_reason;
} CoopTimelineDiagnostics;

typedef struct {
    CoopRemoteSnapshot frames[COOP_TIMELINE_CAPACITY];
    unsigned int count;
    uint64_t last_now;
    /* Preserve packet identity when refusing rollback/mutation. Otherwise the
       same rejected mailbox could seed an empty history on the next tick. */
    CoopRemoteSnapshot newest;
    unsigned int have_newest;
    CoopTimelineConfig config;
    unsigned int configured;
    uint32_t intervals[COOP_TIMELINE_INTERVAL_CAPACITY];
    unsigned int interval_count, interval_next;
    unsigned int increase_votes, decrease_votes, underrun_since_sample;
    uint64_t last_render_now;
    CoopTimelineDiagnostics diagnostics;
} CoopRemoteTimeline;
typedef struct {
    CoopRemoteSnapshot authority; /* Fresh current authority, never synthesized. */
    float position[3];            /* Receiver-local render/movement target. */
    unsigned int interpolated;
    float heading;               /* Delayed shortest-arc presentation angle. */
    unsigned int heading_valid;  /* No invented angle across an unknown pose. */
    uint64_t presentation_ms;    /* Receive-clock pose time, not sender time. */
} CoopRemoteFrame;

/* Game-thread only; zero initialize once. Every call must supply the CURRENT
   CoopNetworkReadRemoteWorld result, including NULL on withdrawal. No cached
   authority, extrapolation, game calls, teleports or packet generation.
   Render position AND heading from this frame; authority always remains the
   exact current snapshot and must not be replaced with the delayed pose.
   No interpolation across large position jumps; the new pose is the anchor. */
int CoopRemoteTimelineAt(CoopRemoteTimeline *timeline,
    const CoopRemoteSnapshot *current, uint64_t now_ms, CoopRemoteFrame *out);

/* All APIs are game-thread only. Zero-init preserves fixed-100 legacy calls.
   Validate every field, including currently unused ones; invalid config leaves
   the instance untouched. Identical configuration is a no-op. Changed config
   resets history/statistics/playback, retaining accepted-packet watermark.
   Reset likewise retains config/watermark and lifetime diagnostics. Neither
   grants freshness or permission to reuse a cached snapshot. A new scope or
   receiver withdrawal still clears stream identity as in PR21. */
void CoopRemoteTimelineDefaultConfig(CoopTimelineConfig *out);
int CoopRemoteTimelineConfigure(CoopRemoteTimeline *timeline, const CoopTimelineConfig *config);
void CoopRemoteTimelineReset(CoopRemoteTimeline *timeline);
int CoopRemoteTimelineReadDiagnostics(const CoopRemoteTimeline *timeline,
    CoopTimelineDiagnostics *out);
#endif
