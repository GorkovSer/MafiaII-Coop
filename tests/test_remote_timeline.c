/* Deterministic production-module tests: no sockets, game process or fake DLL. */
#include "remote_timeline.h"
#include "wire.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    printf("  line %d: %s\n", __LINE__, #expr); return 0; } } while (0)
static int near(float a, float b) { return fabsf(a-b) < 0.0001f; }
static CoopRemoteSnapshot sample(uint64_t ms, uint32_t seq, float x) {
    CoopRemoteSnapshot s={0};
    s.ready=s.active=s.have_state=s.world_authorized=1;
    s.session_id=9; s.player_id=2;
    s.epoch=s.connection_generation=s.world_generation=1;
    s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=ms;
    s.received_sequence=seq; s.position[0]=x;
    return s;
}
/* Baseline compatibility: the original API had only the raw authority angle. */
static float angle(const CoopRemoteFrame *f) {
#ifdef COOP_TIMELINE_RENDER_HEADING
    return f->heading;
#else
    return f->authority.heading;
#endif
}
static unsigned int angle_valid(const CoopRemoteFrame *f) {
#ifdef COOP_TIMELINE_RENDER_HEADING
    return f->heading_valid;
#else
    return f->authority.heading_valid;
#endif
}
static int jitter_and_loss(void) {
    const uint64_t arrival[]={1000,1040,1110,1150,1230,1390,1440};
    const uint32_t sequence[]={1,2,4,5,8,11,12};
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    unsigned int next=0; float previous=0;
    CoopRemoteSnapshot s=sample(arrival[0],sequence[0],0);
    for (uint64_t now=1000; now<=1540; now+=5) {
        if (next<sizeof(arrival)/sizeof(arrival[0]) && now==arrival[next]) {
            s=sample(now,sequence[next],(float)(now-1000)*0.002f); ++next;
        }
        CHECK(CoopRemoteTimelineAt(&t,&s,now,&f));
        CHECK(!memcmp(&f.authority,&s,sizeof(s)));
        CHECK(isfinite(f.position[0]) && f.position[0]>=previous-0.0001f);
        CHECK(f.position[0]>=0 && f.position[0]<=s.position[0]+0.0001f);
        previous=f.position[0];
    }
    CHECK(near(f.position[0],s.position[0]));
    return 1;
}
static int reordered_packets(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot a=sample(1000,10,0), b=sample(1050,12,1);
    CHECK(CoopRemoteTimelineAt(&t,&a,1000,&f));
    CHECK(CoopRemoteTimelineAt(&t,&b,1050,&f));
    a=sample(1060,11,0.5f);
    CHECK(!CoopRemoteTimelineAt(&t,&a,1060,&f) && !f.authority.ready);
    /* The timeline refuses rollback, not an alternative packet receiver. */
    CHECK(CoopRemoteTimelineAt(&t,&b,1070,&f));
    CHECK(near(f.position[0],1) && t.count==1);
    a=sample(1080,13,2);
    CHECK(CoopRemoteTimelineAt(&t,&a,1080,&f));
    return 1;
}
static int sequence_wrap_and_duplicate(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,UINT32_MAX,0);
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=sample(1050,0,1); CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
    CHECK(CoopRemoteTimelineAt(&t,&s,1060,&f) && t.count==2);
    s.position[0]=2;
    CHECK(!CoopRemoteTimelineAt(&t,&s,1070,&f) && !f.authority.ready);
    return 1;
}
static int rollback_cannot_reseed_history(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot latest=sample(1000,10,1), old=sample(1001,9,0);
    CHECK(CoopRemoteTimelineAt(&t,&latest,1000,&f));
    CHECK(!CoopRemoteTimelineAt(&t,&old,1001,&f) && !t.count);
    /* A repeated rejected mailbox must not become a new first anchor. */
    CHECK(!CoopRemoteTimelineAt(&t,&old,1002,&f) && !f.authority.ready);
    CHECK(!CoopRemoteTimelineAt(&t,&old,1003,&f) && !f.authority.ready);
    CHECK(CoopRemoteTimelineAt(&t,&latest,1004,&f) && f.position[0]==1);
    old=latest; old.position[0]=2;
    CHECK(!CoopRemoteTimelineAt(&t,&old,1005,&f));
    CHECK(!CoopRemoteTimelineAt(&t,&old,1006,&f));
    latest=sample(1010,11,1.1f);
    CHECK(CoopRemoteTimelineAt(&t,&latest,1010,&f) && t.count==1);
    return 1;
}
static int burst_same_timestamp(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,1,0);
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    for (uint32_t seq=2;seq<7;++seq) {
        s=sample(1050,seq,(float)seq/10);
        CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
        CHECK(t.count==2 && isfinite(f.position[0]));
    }
    CHECK(CoopRemoteTimelineAt(&t,&s,1125,&f));
    CHECK(near(f.position[0],0.3f));
    return 1;
}
static int heading_delay(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,1,0); s.heading_valid=1; s.heading=0.25f;
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=sample(1050,2,1); s.heading_valid=1; s.heading=1.25f;
    CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
    printf("  buffered heading=%.6f (expected 0.250000)\n",angle(&f));
    CHECK(angle_valid(&f) && near(angle(&f),0.25f));
    CHECK(f.authority.heading==1.25f); /* Must not forge authority. */
    CHECK(CoopRemoteTimelineAt(&t,&s,1125,&f));
    CHECK(near(angle(&f),0.75f) && near(f.position[0],0.5f));
    return 1;
}
static int heading_wrap(void) {
    for (int sign=-1;sign<=1;sign+=2) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f;
        CoopRemoteSnapshot s=sample(1000,1,0); s.heading_valid=1; s.heading=sign*3.05f;
        CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        s=sample(1050,2,0); s.heading_valid=1; s.heading=-sign*3.05f;
        CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
        CHECK(CoopRemoteTimelineAt(&t,&s,1125,&f));
        printf("  wrap midpoint=%.6f (expected magnitude pi)\n",angle(&f));
        CHECK(angle_valid(&f) && near(fabsf(angle(&f)),3.14159265f));
    }
    return 1;
}
static int missing_heading(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,1,0);
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=sample(1050,2,1); s.heading_valid=1; s.heading=1;
    CHECK(CoopRemoteTimelineAt(&t,&s,1125,&f));
    CHECK(!angle_valid(&f) && angle(&f)==0);
    CHECK(CoopRemoteTimelineAt(&t,&s,1150,&f) && angle_valid(&f));
    s=sample(1200,3,1);
    CHECK(CoopRemoteTimelineAt(&t,&s,1225,&f) && !angle_valid(&f));
    return 1;
}
static int stop_no_extrapolation(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s;
    for (uint32_t i=0;i<4;++i) {
        s=sample(1000+i*50,i+1,fminf(1,(float)i*0.5f));
        CHECK(CoopRemoteTimelineAt(&t,&s,s.state_received_ms,&f));
    }
    for (uint64_t now=1250;now<1800;now+=10) {
        CHECK(CoopRemoteTimelineAt(&t,&s,now,&f));
        CHECK(near(f.position[0],1));
    }
    CHECK(!CoopRemoteTimelineAt(&t,&s,1150+COOP_STATE_MAX_AGE_MS,&f));
    CHECK(!t.count && !f.authority.ready && f.position[0]==0);
    return 1;
}
static int abrupt_position_change(void) {
    for (unsigned int axis=0;axis<3;++axis) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f;
        CoopRemoteSnapshot s=sample(1000,1,0);
        CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        s=sample(1050,2,0); s.position[axis]=axis==2 ? 4 : 30;
        CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
        printf("  discontinuity axis %u target=%.3f (expected %.3f)\n",
            axis,f.position[axis],s.position[axis]);
        CHECK(t.count==1 && near(f.position[axis],s.position[axis]));
        CHECK(CoopRemoteTimelineAt(&t,&s,1125,&f) && !f.interpolated);
    }
    return 1;
}
static int receive_stall(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,1,0);
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=sample(1300,2,1);
    CHECK(CoopRemoteTimelineAt(&t,&s,1300,&f) && t.count==1 && f.position[0]==1);
    return 1;
}
static int disconnect_and_new_scope(void) {
    for (unsigned int identity=0;identity<6;++identity) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f;
        CoopRemoteSnapshot s=sample(1000,1,0);
        CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        s=sample(1050,2,1); CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
        s=sample(1060,1,5);
        switch(identity) {
        case 0: ++s.session_id; break;
        case 1: ++s.connection_generation; break;
        case 2: ++s.world_generation; break;
        case 3: ++s.epoch; break;
        case 4: s.player_id=1; break;
        default: s.visual_only=1; break;
        }
        CHECK(CoopRemoteTimelineAt(&t,&s,1060,&f));
        CHECK(t.count==1 && f.position[0]==5 && !f.interpolated);
        CHECK(!CoopRemoteTimelineAt(&t,NULL,1061,&f));
        CHECK(!t.count && !f.authority.ready && f.position[0]==0);
        s=sample(1070,1,7); ++s.session_id;
        CHECK(CoopRemoteTimelineAt(&t,&s,1070,&f) && t.count==1 && f.position[0]==7);
    }
    return 1;
}
static int invalid_and_expired(void) {
    for (unsigned int invalid=0;invalid<8;++invalid) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f;
        CoopRemoteSnapshot s=sample(1000,1,0);
        CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        switch(invalid) {
        case 0: s.active=0; break;
        case 1: s.world_authorized=0; break;
        case 2: s.ready=0; break;
        case 3: s.position[1]=NAN; break;
        case 4: s.heading_valid=1; s.heading=INFINITY; break;
        case 5: s.world_observed_ms=1100; break;
        case 6: s.state_received_ms=1100; break;
        default: s.mailbox_published_ms=1100; break;
        }
        CHECK(!CoopRemoteTimelineAt(&t,&s,1001,&f));
        CHECK(!t.count && !f.authority.ready && f.position[0]==0);
    }
    return 1;
}
static int ring_capacity(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CoopRemoteSnapshot s;
    for (uint32_t i=0;i<40;++i) {
        s=sample(1000+i*25,i+1,(float)i/20);
        CHECK(CoopRemoteTimelineAt(&t,&s,s.state_received_ms,&f));
        CHECK(t.count<=COOP_TIMELINE_CAPACITY);
        if (i>=4) CHECK(near(f.position[0],(float)(i-4)/20));
    }
    return 1;
}
static int alias_and_clock_rollback(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,1,0);
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    CHECK(CoopRemoteTimelineAt(&t,&f.authority,1001,&f));
    s=sample(1000,2,1);
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f) && t.count==1);
    CHECK(!CoopRemoteTimelineAt(&t,&s,999,&f) && !f.authority.ready);
    return 1;
}
static int independent_freshness_clocks(void) {
    for (unsigned int clock=0;clock<3;++clock) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f;
        CoopRemoteSnapshot s=sample(1000,1,0);
        CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        uint64_t now=1000+COOP_STATE_MAX_AGE_MS;
        s=sample(now,2,1);
        if (clock==0) s.state_received_ms=1000;
        else if (clock==1) s.world_observed_ms=1000;
        else {s.mailbox_published_ms=1000; s.state_received_ms=1000; s.world_observed_ms=1000;}
        CHECK(!CoopRemoteTimelineAt(&t,&s,now,&f));
        CHECK(!t.count && !f.authority.ready && !angle_valid(&f));
    }
    return 1;
}
static int boundary_angles_and_steps(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=sample(1000,1,0); s.heading_valid=1; s.heading=COOP_HEADING_MAX;
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=sample(1050,2,20); s.position[2]=3; s.heading_valid=1; s.heading=-COOP_HEADING_MAX;
    CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f) && t.count==2);
    CHECK(CoopRemoteTimelineAt(&t,&s,1100,&f));
    CHECK(angle_valid(&f) && near(angle(&f),COOP_HEADING_MAX));
    CHECK(CoopRemoteTimelineAt(&t,&s,1125,&f));
    CHECK(fabsf(angle(&f))>3.14f && fabsf(angle(&f))<=COOP_HEADING_MAX);
    s=sample(1150,3,40.01f); s.position[2]=3;
    CHECK(CoopRemoteTimelineAt(&t,&s,1150,&f) && t.count==1);
    CHECK(f.position[0]==s.position[0]);
    /* Same-ms replacement must also respect the discontinuity boundary. */
    s.received_sequence=4; s.position[0]=0;
    CHECK(CoopRemoteTimelineAt(&t,&s,1150,&f) && t.count==1 && f.position[0]==0);
    return 1;
}
int main(void) {
    const struct {const char *name; int (*run)(void);} cases[]={
        {"jitter / dropped sequence numbers",jitter_and_loss},
        {"packet reordering refusal",reordered_packets},
        {"sequence wrap / duplicate integrity",sequence_wrap_and_duplicate},
        {"rejected packets cannot reseed history",rollback_cannot_reseed_history},
        {"same-ms delivery burst",burst_same_timestamp},
        {"heading shares position delay",heading_delay},
        {"shortest turn through -pi/pi",heading_wrap},
        {"unknown heading is not invented",missing_heading},
        {"stop / no extrapolation / exact TTL",stop_no_extrapolation},
        {"position discontinuity",abrupt_position_change},
        {"receive stall resets history",receive_stall},
        {"disconnect / six scope boundaries",disconnect_and_new_scope},
        {"invalid and future snapshots",invalid_and_expired},
        {"ring capacity",ring_capacity},
        {"alias / receiver clock rollback",alias_and_clock_rollback},
        {"independent state / world / mailbox expiry",independent_freshness_clocks},
        {"angle endpoints / step boundaries",boundary_angles_and_steps}
    };
    unsigned int failed=0;
    for (unsigned int i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        int ok=cases[i].run(); printf("%s: %s\n",ok ? "PASS" : "FAIL",cases[i].name);
        failed+=!ok;
    }
    printf("timeline scenarios: %u passed, %u failed\n",
        (unsigned int)(sizeof(cases)/sizeof(cases[0]))-failed,failed);
    return failed ? 1 : 0;
}
