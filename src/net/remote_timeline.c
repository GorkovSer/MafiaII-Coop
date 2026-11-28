#include "remote_timeline.h"
#include "wire.h"
#include <math.h>
#include <string.h>

static int same_world(const CoopRemoteSnapshot *a, const CoopRemoteSnapshot *b) {
    return a->session_id == b->session_id && a->connection_generation == b->connection_generation &&
        a->world_generation == b->world_generation && a->epoch == b->epoch && a->player_id == b->player_id &&
        a->visual_only == b->visual_only;
}
static int position_jump(const CoopRemoteSnapshot *a, const CoopRemoteSnapshot *b) {
    float dx=b->position[0]-a->position[0], dy=b->position[1]-a->position[1];
    return dx*dx+dy*dy > COOP_TIMELINE_MAX_STEP_XY*COOP_TIMELINE_MAX_STEP_XY ||
        fabsf(b->position[2]-a->position[2]) > COOP_TIMELINE_MAX_STEP_Z;
}
static void pose(CoopRemoteFrame *out, const CoopRemoteSnapshot *s) {
    memcpy(out->position, s->position, sizeof(out->position));
    out->heading=s->heading;
    out->heading_valid=s->heading_valid;
}
static void interpolate_heading(CoopRemoteFrame *out,
    const CoopRemoteSnapshot *a, const CoopRemoteSnapshot *b, float f) {
    const float pi=3.14159265358979323846f, tau=6.28318530717958647692f;
    out->heading=0; out->heading_valid=0;
    /* Exact endpoints do not require the neighbouring packet to have facing. */
    if (f<=0) {out->heading=a->heading; out->heading_valid=a->heading_valid; return;}
    if (f>=1) {out->heading=b->heading; out->heading_valid=b->heading_valid; return;}
    if (!a->heading_valid || !b->heading_valid) return;
    float delta=b->heading-a->heading;
    if (delta>pi) delta-=tau;
    else if (delta < -pi) delta+=tau;
    float heading=a->heading+delta*f;
    if (heading>pi) heading-=tau;
    else if (heading < -pi) heading+=tau;
    out->heading=heading; out->heading_valid=1;
}
static void increment(uint64_t *counter) {
    if (*counter!=UINT64_MAX) ++*counter;
}
void CoopRemoteTimelineDefaultConfig(CoopTimelineConfig *out) {
    if (!out) return;
    *out=(CoopTimelineConfig){COOP_INTERPOLATION_FIXED,100,100,200,8};
}
static int config_valid(const CoopTimelineConfig *c) {
    return c && c->mode<=COOP_INTERPOLATION_ADAPTIVE &&
        c->fixed_delay_ms<=COOP_TIMELINE_DELAY_LIMIT_MS && c->min_delay_ms>=50 &&
        c->min_delay_ms<=c->max_delay_ms && c->max_delay_ms<=COOP_TIMELINE_DELAY_LIMIT_MS &&
        c->warmup_intervals>=4 && c->warmup_intervals<=COOP_TIMELINE_INTERVAL_CAPACITY;
}
static int config_equal(const CoopTimelineConfig *a, const CoopTimelineConfig *b) {
    return a->mode==b->mode && a->fixed_delay_ms==b->fixed_delay_ms &&
        a->min_delay_ms==b->min_delay_ms && a->max_delay_ms==b->max_delay_ms &&
        a->warmup_intervals==b->warmup_intervals;
}
static void ensure_config(CoopRemoteTimeline *t) {
    if (t->configured) return;
    CoopRemoteTimelineDefaultConfig(&t->config);
    t->configured=1;
    t->diagnostics.mode=t->config.mode;
    t->diagnostics.delay_ms=t->config.fixed_delay_ms;
    t->diagnostics.warming=1;
}
static void reset_stream(CoopRemoteTimeline *t, CoopTimelineResetReason reason,
    int keep_watermark, int keep_delivery) {
    memset(t->frames,0,sizeof(t->frames)); t->count=0;
    if (!keep_watermark) {memset(&t->newest,0,sizeof(t->newest)); t->have_newest=0;}
    memset(t->intervals,0,sizeof(t->intervals));
    t->interval_count=t->interval_next=0;
    t->increase_votes=t->decrease_votes=t->underrun_since_sample=0;
    t->last_render_now=0;
    t->diagnostics.presentation_valid=0; t->diagnostics.presentation_ms=0;
    t->diagnostics.effective_delay_ms=0;
    t->diagnostics.holding=0; t->diagnostics.warming=1;
    if (!keep_delivery) {
        memset(&t->diagnostics.delivery,0,sizeof(t->diagnostics.delivery));
        t->diagnostics.delay_ms=t->config.mode==COOP_INTERPOLATION_FIXED ?
            t->config.fixed_delay_ms : t->config.min_delay_ms;
    }
    t->diagnostics.delivery.window_intervals=0;
    t->diagnostics.delivery.mean_interval_us=t->diagnostics.delivery.variation_us=0;
    t->diagnostics.last_reset_reason=(unsigned int)reason;
    increment(&t->diagnostics.resets[reason]);
}
int CoopRemoteTimelineConfigure(CoopRemoteTimeline *t, const CoopTimelineConfig *config) {
    if (!t || !config_valid(config)) return 0;
    CoopTimelineConfig c=*config; /* Also permit config==&timeline->config. */
    if (t->configured && config_equal(&t->config,&c)) return 1;
    t->config=c; t->configured=1; t->diagnostics.mode=c.mode;
    reset_stream(t,COOP_TIMELINE_RESET_CONFIG,1,0); t->last_now=0;
    return 1;
}
void CoopRemoteTimelineReset(CoopRemoteTimeline *t) {
    if (!t) return;
    ensure_config(t);
    reset_stream(t,COOP_TIMELINE_RESET_EXPLICIT,1,0); t->last_now=0;
}
int CoopRemoteTimelineReadDiagnostics(const CoopRemoteTimeline *t, CoopTimelineDiagnostics *out) {
    if (!out) return 0;
    if (!t) {memset(out,0,sizeof(*out)); return 0;}
    *out=t->diagnostics;
    out->snapshot_count=t->count;
    if (!t->configured) {
        out->mode=COOP_INTERPOLATION_FIXED; out->delay_ms=COOP_TIMELINE_DELAY_MS;
        out->warming=1;
    }
    return 1;
}
/* Candidate policy, not a measured network estimator. Decisions are made once
   per NEW positive receive interval, never per render tick or same-ms burst. */
static void adapt_delay(CoopRemoteTimeline *t) {
    if (t->config.mode!=COOP_INTERPOLATION_ADAPTIVE ||
        t->interval_count<t->config.warmup_intervals) return;
    CoopTimelineDeliveryStats *d=&t->diagnostics.delivery;
    unsigned int wanted=(d->mean_interval_us+2*d->variation_us+20000+999)/1000;
    if (wanted<t->config.min_delay_ms) wanted=t->config.min_delay_ms;
    if (wanted>t->config.max_delay_ms) wanted=t->config.max_delay_ms;
    unsigned int delay=t->diagnostics.delay_ms;
    /* Repeated starvation can request one bounded step even if the window's
       average is low. It still needs three qualifying accepted intervals. */
    if (t->underrun_since_sample && wanted<delay+10) {
        wanted=delay+10;
        if (wanted>t->config.max_delay_ms) wanted=t->config.max_delay_ms;
    }
    if (wanted>=delay+10) {
        t->decrease_votes=0;
        if (++t->increase_votes>=3) {
            t->diagnostics.delay_ms=delay+10; t->increase_votes=0;
        }
    } else if (wanted+20<=delay) {
        t->increase_votes=0;
        if (++t->decrease_votes>=20) {
            unsigned int lowered=delay-5;
            t->diagnostics.delay_ms=lowered<t->config.min_delay_ms ? t->config.min_delay_ms : lowered;
            t->decrease_votes=0;
        }
    } else {t->increase_votes=t->decrease_votes=0;}
}
static void observe_interval(CoopRemoteTimeline *t, uint64_t interval) {
    CoopTimelineDeliveryStats *d=&t->diagnostics.delivery;
    d->last_interval_ms=interval>UINT32_MAX ? UINT32_MAX : (unsigned int)interval;
    if (!interval) {increment(&d->same_ms_bursts); return;}
    increment(&d->positive_intervals);
    if (interval>COOP_TIMELINE_MAX_GAP_MS) {
        increment(&d->long_gaps); t->underrun_since_sample=0; return;
    }
    t->intervals[t->interval_next]=(uint32_t)interval;
    t->interval_next=(t->interval_next+1)%COOP_TIMELINE_INTERVAL_CAPACITY;
    if (t->interval_count<COOP_TIMELINE_INTERVAL_CAPACITY) ++t->interval_count;
    unsigned int sum=0;
    for (unsigned int i=0;i<t->interval_count;++i) sum+=t->intervals[i]*1000u;
    d->mean_interval_us=sum/t->interval_count;
    unsigned int deviation=0;
    for (unsigned int i=0;i<t->interval_count;++i) {
        unsigned int value=t->intervals[i]*1000u;
        deviation+=value>d->mean_interval_us ? value-d->mean_interval_us : d->mean_interval_us-value;
    }
    d->variation_us=deviation/t->interval_count; d->window_intervals=t->interval_count;
    adapt_delay(t); t->underrun_since_sample=0;
}
static void append(CoopRemoteTimeline *t, const CoopRemoteSnapshot *s) {
    if (t->count && s->state_received_ms==t->frames[t->count-1].state_received_ms) {
        t->frames[t->count-1]=*s; return;
    }
    if (t->count==COOP_TIMELINE_CAPACITY) {
        unsigned int remove=0;
        /* In adaptive mode keep the lower playback anchor if all subsequent
           frames are still in its future. Downsample one interior point rather
           than forcing the playhead forward to a new oldest timestamp. */
        if (t->config.mode==COOP_INTERPOLATION_ADAPTIVE && t->diagnostics.presentation_valid &&
            t->frames[1].state_received_ms>t->diagnostics.presentation_ms) remove=1;
        memmove(t->frames+remove,t->frames+remove+1,
            (COOP_TIMELINE_CAPACITY-remove-1)*sizeof(t->frames[0]));
        --t->count; increment(&t->diagnostics.delivery.history_evictions);
    }
    t->frames[t->count++]=*s;
    if (t->count>t->diagnostics.delivery.history_peak) t->diagnostics.delivery.history_peak=t->count;
}
static uint64_t add_saturating(uint64_t a, uint64_t b) {return b>UINT64_MAX-a ? UINT64_MAX : a+b;}
static uint64_t presentation_time(CoopRemoteTimeline *t, uint64_t now) {
    CoopTimelineDiagnostics *d=&t->diagnostics;
    uint64_t target=now>=d->delay_ms ? now-d->delay_ms : 0;
    uint64_t oldest=t->frames[0].state_received_ms, newest=t->frames[t->count-1].state_received_ms;
    unsigned int warming=(target<oldest || t->count<2);
    if (t->config.mode==COOP_INTERPOLATION_ADAPTIVE) {
        warming=warming || t->interval_count<t->config.warmup_intervals;
        if (d->presentation_valid) {
            if (target<d->presentation_ms) target=d->presentation_ms;
            uint64_t elapsed=now-t->last_render_now;
            uint64_t budget=add_saturating(elapsed,elapsed/4);
            uint64_t limit=add_saturating(d->presentation_ms,budget);
            if (target>limit) target=limit; /* At most 1.25x catch-up, no rewind. */
        }
    }
    if (target<oldest) target=oldest;
    if (target>newest) target=newest; /* Hold, never extrapolate. */
    unsigned int holding=(target==newest && now>newest && now-newest>=d->delay_ms);
    if (holding) {
        if (!d->holding) increment(&d->delivery.underrun_events);
        t->underrun_since_sample=1;
    }
    d->holding=holding; d->warming=warming;
    d->presentation_ms=target; d->presentation_valid=1;
    uint64_t age=now-target;
    d->effective_delay_ms=age>UINT32_MAX ? UINT32_MAX : (unsigned int)age;
    t->last_render_now=now;
    return target;
}
static int expired(const CoopRemoteSnapshot *s, uint64_t now) {
    return (now>=s->mailbox_published_ms && now-s->mailbox_published_ms>=COOP_STATE_MAX_AGE_MS) ||
        (now>=s->state_received_ms && now-s->state_received_ms>=COOP_STATE_MAX_AGE_MS) ||
        (now>=s->world_observed_ms && now-s->world_observed_ms>=COOP_STATE_MAX_AGE_MS);
}
static int refuse_packet(CoopRemoteTimeline *t, CoopTimelineRefusalReason reason, uint64_t now) {
    increment(&t->diagnostics.refusals[reason]); t->diagnostics.last_refusal_reason=(unsigned int)reason;
    if (t->count || t->diagnostics.presentation_valid)
        reset_stream(t,COOP_TIMELINE_RESET_REJECTED,1,1);
    t->last_now=now; return 0;
}
int CoopRemoteTimelineAt(CoopRemoteTimeline *t, const CoopRemoteSnapshot *current,
    uint64_t now, CoopRemoteFrame *out) {
    if (!out) return 0;
    CoopRemoteSnapshot raw={0}; if (current) raw=*current;
    memset(out,0,sizeof(*out));
    if (!t) return 0;
    ensure_config(t);
    CoopRemoteSnapshot s;
    if (!CoopRemoteSnapshotAt(current ? &raw : NULL,now,&s) || !s.active || !s.world_authorized) {
        CoopTimelineRefusalReason refusal=COOP_TIMELINE_REFUSAL_INVALID;
        CoopTimelineResetReason reset=COOP_TIMELINE_RESET_INVALID;
        if (!current || !raw.ready || !raw.active || !raw.world_authorized) {
            refusal=COOP_TIMELINE_REFUSAL_WITHDRAWAL; reset=COOP_TIMELINE_RESET_WITHDRAWAL;
        } else if (expired(&raw,now)) {
            refusal=COOP_TIMELINE_REFUSAL_EXPIRED; reset=COOP_TIMELINE_RESET_EXPIRED;
        }
        increment(&t->diagnostics.refusals[refusal]);
        t->diagnostics.last_refusal_reason=(unsigned int)refusal;
        if (t->have_newest || t->count) reset_stream(t,reset,0,0);
        t->last_now=0; return 0;
    }
    if (t->count>COOP_TIMELINE_CAPACITY) reset_stream(t,COOP_TIMELINE_RESET_CORRUPT,0,0);
    if (now<t->last_now) reset_stream(t,COOP_TIMELINE_RESET_CLOCK,0,0);
    if (t->have_newest && !same_world(&t->newest,&s)) reset_stream(t,COOP_TIMELINE_RESET_SCOPE,0,0);
    t->last_now=now;
    unsigned int new_packet=!t->have_newest || s.received_sequence!=t->newest.received_sequence;
    uint64_t interval=0; unsigned int had_newest=t->have_newest;
    if (had_newest) {
        const CoopRemoteSnapshot *last=&t->newest;
        if (new_packet) {
            if (!CoopSequenceNewer(s.received_sequence,last->received_sequence))
                return refuse_packet(t,COOP_TIMELINE_REFUSAL_SEQUENCE,now);
            if (s.state_received_ms<last->state_received_ms)
                return refuse_packet(t,COOP_TIMELINE_REFUSAL_TIMESTAMP,now);
            interval=s.state_received_ms-last->state_received_ms;
        } else if (s.state_received_ms!=last->state_received_ms ||
            memcmp(s.position,last->position,sizeof(s.position)) ||
            s.heading_valid!=last->heading_valid || s.heading!=last->heading) {
            return refuse_packet(t,COOP_TIMELINE_REFUSAL_DUPLICATE,now);
        }
        if (new_packet && position_jump(last,&s)) reset_stream(t,COOP_TIMELINE_RESET_POSITION,1,1);
        else if (new_packet && interval>COOP_TIMELINE_MAX_GAP_MS)
            reset_stream(t,COOP_TIMELINE_RESET_GAP,1,1);
    }
    if (new_packet) {
        increment(&t->diagnostics.delivery.accepted_snapshots);
        if (had_newest) observe_interval(t,interval);
    }
    t->newest=s; t->have_newest=1;
    if (new_packet || !t->count) append(t,&s);
    out->authority=s; pose(out,&s);
    uint64_t target=presentation_time(t,now); out->presentation_ms=target;
    if (t->count<2 || target>=s.state_received_ms) return 1;
    if (target<t->frames[0].state_received_ms) {pose(out,&t->frames[0]); return 1;}
    for (unsigned int i=1;i<t->count;++i) {
        const CoopRemoteSnapshot *a=&t->frames[i-1], *b=&t->frames[i];
        if (target<a->state_received_ms || target>b->state_received_ms) continue;
        uint64_t gap=b->state_received_ms-a->state_received_ms;
        /* Adaptive overflow can deliberately downsample an interior point.
           Every actual receive stall was already cut before append(). */
        if (!gap || (t->config.mode==COOP_INTERPOLATION_FIXED && gap>COOP_TIMELINE_MAX_GAP_MS)) return 1;
        float f=(float)(target-a->state_received_ms)/(float)gap;
        for (unsigned int axis=0;axis<3;++axis)
            out->position[axis]=a->position[axis]+(b->position[axis]-a->position[axis])*f;
        interpolate_heading(out,a,b,f); out->interpolated=1; return 1;
    }
    return 1;
}
