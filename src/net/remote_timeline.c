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
int CoopRemoteTimelineAt(CoopRemoteTimeline *t, const CoopRemoteSnapshot *current,
    uint64_t now, CoopRemoteFrame *out) {
    if (!out) return 0;
    CoopRemoteSnapshot raw = {0};
    if (current) raw = *current;
    memset(out, 0, sizeof(*out));
    if (!t) return 0;
    CoopRemoteSnapshot s;
    if (!CoopRemoteSnapshotAt(current ? &raw : NULL, now, &s) || !s.active || !s.world_authorized) {
        memset(t, 0, sizeof(*t)); return 0;
    }
    if (t->count > COOP_TIMELINE_CAPACITY || now < t->last_now ||
        (t->have_newest && !same_world(&t->newest, &s)))
        memset(t, 0, sizeof(*t));
    t->last_now = now;
    if (t->have_newest) {
        const CoopRemoteSnapshot *last = &t->newest;
        if (s.received_sequence != last->received_sequence) {
            if (!CoopSequenceNewer(s.received_sequence, last->received_sequence) ||
                s.state_received_ms < last->state_received_ms) {
                /* A rollback is not fresh interpolation history. */
                t->count=0; return 0;
            }
        } else if (s.state_received_ms != last->state_received_ms ||
            memcmp(s.position, last->position, sizeof(s.position)) ||
            s.heading_valid != last->heading_valid || s.heading != last->heading) {
            t->count=0; return 0;
        }
        if (s.received_sequence != last->received_sequence && position_jump(last,&s))
            t->count=0; /* Do not render a fast sweep across a relocation. */
    }
    t->newest=s; t->have_newest=1;
    /* A receiver clock can timestamp a burst of packets in the same millisecond.
       Keep the newest sequence without creating a zero-length interval. */
    if (t->count && s.received_sequence != t->frames[t->count-1].received_sequence &&
        s.state_received_ms == t->frames[t->count-1].state_received_ms)
        t->frames[t->count-1] = s;
    if (!t->count || s.received_sequence != t->frames[t->count-1].received_sequence) {
        if (t->count && s.state_received_ms-t->frames[t->count-1].state_received_ms > COOP_TIMELINE_MAX_GAP_MS)
            t->count=0; /* Never bridge a receive stall with invented motion. */
        if (t->count == COOP_TIMELINE_CAPACITY) {
            memmove(t->frames, t->frames+1, (COOP_TIMELINE_CAPACITY-1)*sizeof(t->frames[0]));
            --t->count;
        }
        t->frames[t->count++] = s;
    }
    out->authority = s;
    pose(out,&s);
    uint64_t target = now >= COOP_TIMELINE_DELAY_MS ? now-COOP_TIMELINE_DELAY_MS : 0;
    if (t->count < 2 || target >= s.state_received_ms) return 1;
    if (target < t->frames[0].state_received_ms) {
        pose(out,&t->frames[0]);
        return 1;
    }
    for (unsigned int i=1; i<t->count; ++i) {
        const CoopRemoteSnapshot *a=&t->frames[i-1], *b=&t->frames[i];
        if (target < a->state_received_ms || target > b->state_received_ms) continue;
        uint64_t gap=b->state_received_ms-a->state_received_ms;
        if (!gap || gap > COOP_TIMELINE_MAX_GAP_MS) return 1;
        float f=(float)(target-a->state_received_ms)/(float)gap;
        for (unsigned int axis=0; axis<3; ++axis)
            out->position[axis]=a->position[axis]+(b->position[axis]-a->position[axis])*f;
        interpolate_heading(out,a,b,f);
        out->interpolated=1; return 1;
    }
    /* Not enough history yet: use current finite position without prediction. */
    return 1;
}
