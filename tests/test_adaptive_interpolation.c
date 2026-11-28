/* Written for future execution. No results are implied by the PASS labels. */
#include "remote_timeline.h"
#include "wire.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){printf("line %d: %s\n",__LINE__,#x);return 0;}}while(0)
static int near(float a,float b) {return fabsf(a-b)<0.0001f;}
static CoopRemoteSnapshot packet(uint64_t ms,uint32_t seq,float x) {
    CoopRemoteSnapshot s={0};
    s.ready=s.active=s.have_state=s.world_authorized=1;
    s.session_id=9; s.connection_generation=s.world_generation=s.epoch=1; s.player_id=2;
    s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=ms;
    s.received_sequence=seq; s.position[0]=x; return s;
}
static int adaptive(CoopRemoteTimeline *t) {
    CoopTimelineConfig c; CoopRemoteTimelineDefaultConfig(&c); c.mode=COOP_INTERPOLATION_ADAPTIVE;
    return CoopRemoteTimelineConfigure(t,&c);
}
static CoopTimelineDiagnostics diagnostics(const CoopRemoteTimeline *t) {
    CoopTimelineDiagnostics d={0}; CoopRemoteTimelineReadDiagnostics(t,&d); return d;
}
static int configuration(void) {
    CoopRemoteTimeline t={0},copy; CoopTimelineConfig c; CoopRemoteTimelineDefaultConfig(&c);
    CHECK(c.mode==COOP_INTERPOLATION_FIXED && c.fixed_delay_ms==100);
    for (unsigned int i=0;i<7;++i) {
        CoopTimelineConfig bad=c;
        switch(i) {
        case 0: bad.mode=2; break; case 1: bad.fixed_delay_ms=201; break;
        case 2: bad.min_delay_ms=49; break; case 3: bad.max_delay_ms=201; break;
        case 4: bad.min_delay_ms=150; bad.max_delay_ms=100; break;
        case 5: bad.warmup_intervals=3; break; default: bad.warmup_intervals=17; break;
        }
        copy=t; CHECK(!CoopRemoteTimelineConfigure(&t,&bad)); CHECK(!memcmp(&t,&copy,sizeof(t)));
    }
    CHECK(!CoopRemoteTimelineConfigure(&t,NULL));
    CHECK(CoopRemoteTimelineConfigure(&t,&c));
    CoopRemoteFrame f; CoopRemoteSnapshot s=packet(1000,10,1);
    CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f)); copy=t;
    CoopTimelineConfig invalid_live=c; invalid_live.max_delay_ms=201;
    CHECK(!CoopRemoteTimelineConfigure(&t,&invalid_live) && !memcmp(&t,&copy,sizeof(t)));
    CHECK(CoopRemoteTimelineConfigure(&t,&c) && !memcmp(&t,&copy,sizeof(t)));
    c.mode=COOP_INTERPOLATION_ADAPTIVE;
    CHECK(CoopRemoteTimelineConfigure(&t,&c));
    CHECK(diagnostics(&t).snapshot_count==0 && diagnostics(&t).delay_ms==100);
    CoopRemoteSnapshot old=packet(1001,9,0);
    CHECK(!CoopRemoteTimelineAt(&t,&old,1001,&f));
    CHECK(CoopRemoteTimelineAt(&t,&s,1002,&f) && f.position[0]==1);
    CoopRemoteTimelineReset(&t);
    CHECK(diagnostics(&t).mode==COOP_INTERPOLATION_ADAPTIVE);
    CHECK(!CoopRemoteTimelineAt(&t,&old,1003,&f));
    return 1;
}
static int stable_20hz(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    uint64_t previous=0;
    for (uint32_t i=0;i<80;++i) {
        CoopRemoteSnapshot s=packet(1000+i*50,i+1,(float)i*0.1f);
        CHECK(CoopRemoteTimelineAt(&t,&s,s.state_received_ms,&f));
        CHECK(f.presentation_ms>=previous); previous=f.presentation_ms;
        CoopTimelineDiagnostics d=diagnostics(&t);
        CHECK(d.delay_ms==100 && d.snapshot_count<=COOP_TIMELINE_CAPACITY);
        if (i>=8) CHECK(!d.warming);
        CHECK(!memcmp(&f.authority,&s,sizeof(s)));
        if (i>=2) CHECK(near(f.position[0],(float)(i-2)*0.1f));
    }
    return 1;
}
static int repeat_and_readonly(void) {
    CoopRemoteTimeline t={0},copy; CoopRemoteFrame f; CHECK(adaptive(&t));
    CoopRemoteSnapshot s=packet(1000,1,0); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=packet(1050,2,0.1f); CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
    for (uint64_t now=1050;now<1100;++now) CHECK(CoopRemoteTimelineAt(&t,&s,now,&f));
    copy=t; CoopTimelineDiagnostics d=diagnostics(&t);
    CHECK(!memcmp(&t,&copy,sizeof(t)));
    CHECK(d.delivery.accepted_snapshots==2 && d.delivery.positive_intervals==1);
    CHECK(d.delivery.window_intervals==1 && d.delivery.mean_interval_us==50000);
    CHECK(d.delivery.same_ms_bursts==0);
    return 1;
}
static int bursts_and_hysteresis(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    CoopRemoteSnapshot s=packet(1000,1,0); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    for (uint32_t i=2;i<=30;++i) {
        s=packet(1000,i,(float)i*0.01f); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    }
    CoopTimelineDiagnostics d=diagnostics(&t);
    CHECK(d.delay_ms==100 && d.delivery.same_ms_bursts==29);
    CHECK(d.delivery.positive_intervals==0 && d.snapshot_count==1);
    /* One delivery anomaly does not produce an immediate policy jump. */
    s=packet(1200,31,0.4f); CHECK(CoopRemoteTimelineAt(&t,&s,1200,&f));
    CHECK(diagnostics(&t).delay_ms==100);
    return 1;
}
static int jitter_recovery_and_instances(void) {
    CoopRemoteTimeline a={0},b={0}; CoopRemoteFrame f,g; CHECK(adaptive(&a));
    uint64_t now=1000,previous=0; uint32_t seq=0;
    for (unsigned int i=0;i<80;++i) {
        now+=i%2 ? 140 : 20;
        CoopRemoteSnapshot s=packet(now,++seq,(float)(now-1000)*0.002f);
        CHECK(CoopRemoteTimelineAt(&a,&s,now,&f));
        CHECK(CoopRemoteTimelineAt(&b,&s,now,&g));
        CHECK(f.presentation_ms>=previous); previous=f.presentation_ms;
        CHECK(diagnostics(&a).delay_ms>=100 && diagnostics(&a).delay_ms<=200);
        CHECK(diagnostics(&b).delay_ms==100);
    }
    CHECK(diagnostics(&a).delay_ms>100);
    unsigned int previous_delay=diagnostics(&a).delay_ms;
    for (unsigned int i=0;i<520;++i) {
        now+=50; CoopRemoteSnapshot s=packet(now,++seq,(float)(now-1000)*0.002f);
        CHECK(CoopRemoteTimelineAt(&a,&s,now,&f));
        CHECK(f.presentation_ms>=previous); previous=f.presentation_ms;
        unsigned int delay=diagnostics(&a).delay_ms;
        CHECK(delay>=100 && delay<=200);
        /* Once the jitter window is replaced, recovery is gradual. */
        if (i>20) CHECK(delay<=previous_delay && previous_delay-delay<=5);
        previous_delay=delay;
    }
    CHECK(diagnostics(&a).delay_ms==100);
    CHECK(diagnostics(&b).delivery.accepted_snapshots==80);
    return 1;
}
static int isolated_anomaly_after_warmup(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    uint64_t now=1000;
    for (uint32_t i=0;i<60;++i) {
        if(i) now+=i==20 ? 200 : 50;
        CoopRemoteSnapshot s=packet(now,i+1,(float)(now-1000)*0.002f);
        CHECK(CoopRemoteTimelineAt(&t,&s,now,&f));
        CHECK(diagnostics(&t).delay_ms==100);
    }
    return 1;
}
static int render_clock_is_monotonic(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    uint64_t next=1000,previous=0; uint32_t seq=0; CoopRemoteSnapshot s={0};
    unsigned int rises=0; unsigned int last_delay=100;
    for (uint64_t now=1000;now<=16000;now+=10) {
        if (now==next) {
            s=packet(now,++seq,(float)(now-1000)*0.002f);
            next+=seq%2 ? 20 : 140;
        }
        CHECK(CoopRemoteTimelineAt(&t,&s,now,&f));
        CHECK(f.presentation_ms>=previous);
        CHECK(f.presentation_ms<=s.state_received_ms && f.presentation_ms<=now);
        if (previous) CHECK(f.presentation_ms-previous<=12); /* 10ms + floor(10/4). */
        previous=f.presentation_ms;
        unsigned int delay=diagnostics(&t).delay_ms;
        rises+=delay>last_delay; last_delay=delay;
    }
    CHECK(rises>0); return 1;
}
static int underflow_gap_and_burst_return(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    CoopRemoteSnapshot s=packet(1000,1,0); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=packet(1050,2,1); CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
    for (uint64_t now=1150;now<=1300;now+=10) {
        CHECK(CoopRemoteTimelineAt(&t,&s,now,&f) && f.position[0]<=1);
    }
    CHECK(diagnostics(&t).holding && diagnostics(&t).delivery.underrun_events==1);
    s=packet(1400,3,2); CHECK(CoopRemoteTimelineAt(&t,&s,1400,&f));
    CHECK(f.position[0]==2 && diagnostics(&t).last_reset_reason==COOP_TIMELINE_RESET_GAP);
    for (uint32_t seq=4;seq<12;++seq) {
        s=packet(1400,seq,2); CHECK(CoopRemoteTimelineAt(&t,&s,1400,&f));
    }
    CHECK(diagnostics(&t).delivery.long_gaps==1);
    CHECK(diagnostics(&t).delay_ms==100 && diagnostics(&t).delivery.same_ms_bursts==8);
    CHECK(!CoopRemoteTimelineAt(&t,&s,3400,&f) && !f.authority.ready);
    return 1;
}
static int stop_and_restart(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    for (uint32_t i=0;i<40;++i) {
        float x=i<10 ? (float)i*0.1f : i<25 ? 0.9f : 0.9f+(float)(i-24)*0.1f;
        CoopRemoteSnapshot s=packet(1000+i*50,i+1,x);
        CHECK(CoopRemoteTimelineAt(&t,&s,s.state_received_ms,&f));
        if (i>=12 && i<25) CHECK(near(f.position[0],0.9f));
        if (i>=28) CHECK(f.position[0]>0.9f && f.position[0]<=x);
    }
    return 1;
}
static int available_latency_bound(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    CoopRemoteSnapshot s=packet(1000,1,0); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    for (uint64_t now=1005;now<1240;now+=5) CHECK(CoopRemoteTimelineAt(&t,&s,now,&f));
    uint64_t previous=f.presentation_ms;
    s=packet(1240,2,0.24f); /* Shorter than a hard 250ms receive-gap reset. */
    CHECK(CoopRemoteTimelineAt(&t,&s,1240,&f));
    CHECK(f.presentation_ms>=previous && f.presentation_ms>=1040);
    CHECK(diagnostics(&t).effective_delay_ms<=200);
    CHECK(diagnostics(&t).delivery.latency_clamps>0);
    return 1;
}
static int angle_and_relocation(void) {
    for (unsigned int mode=0;mode<2;++mode) for (int sign=-1;sign<=1;sign+=2) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f;
        if (mode) CHECK(adaptive(&t));
        CoopRemoteSnapshot s=packet(1000,1,0); s.heading_valid=1; s.heading=sign*3.05f;
        CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        s=packet(1050,2,1); s.heading_valid=1; s.heading=-sign*3.05f;
        CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
        CHECK(CoopRemoteTimelineAt(&t,&s,1125,&f));
        CHECK(f.heading_valid && near(fabsf(f.heading),3.14159265f));
        s=packet(1150,3,1); /* Unknown facing at newest endpoint. */
        CHECK(CoopRemoteTimelineAt(&t,&s,1150,&f));
        CHECK(f.heading_valid); /* Exact previous known endpoint, not invented. */
        CHECK(CoopRemoteTimelineAt(&t,&s,1200,&f) && !f.heading_valid);
        s=packet(1250,4,31); s.heading_valid=1; s.heading=0.5f;
        CHECK(CoopRemoteTimelineAt(&t,&s,1250,&f));
        CHECK(f.position[0]==31 && f.heading==0.5f && diagnostics(&t).snapshot_count==1);
        s=packet(1300,5,31); s.position[2]=4; s.heading_valid=1; s.heading=-0.5f;
        CHECK(CoopRemoteTimelineAt(&t,&s,1300,&f));
        CHECK(f.position[2]==4 && f.heading==-0.5f && diagnostics(&t).snapshot_count==1);
    }
    return 1;
}
static int scope_and_withdrawal(void) {
    for (unsigned int key=0;key<6;++key) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
        CoopRemoteSnapshot s=packet(1000,8,1); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        s=packet(1050,9,2); CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
        s=packet(1060,1,3);
        switch(key) {case 0: ++s.session_id; break; case 1: ++s.connection_generation; break;
        case 2: ++s.world_generation; break; case 3: ++s.epoch; break;
        case 4: s.player_id=1; break; default: s.visual_only=1; break;}
        CHECK(CoopRemoteTimelineAt(&t,&s,1060,&f) && f.position[0]==3);
        CHECK(diagnostics(&t).delivery.accepted_snapshots==1 && diagnostics(&t).delivery.positive_intervals==0);
        CHECK(diagnostics(&t).resets[COOP_TIMELINE_RESET_SCOPE]==1);
        CHECK(!CoopRemoteTimelineAt(&t,NULL,1061,&f) && !f.authority.ready);
        CHECK(!diagnostics(&t).presentation_valid && diagnostics(&t).snapshot_count==0);
        CHECK(diagnostics(&t).delivery.accepted_snapshots==0);
        s=packet(1070,1,4); ++s.session_id;
        CHECK(CoopRemoteTimelineAt(&t,&s,1070,&f) && f.position[0]==4);
    }
    return 1;
}
static int independent_expiry(void) {
    for (unsigned int clock=0;clock<3;++clock) {
        CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
        CoopRemoteSnapshot s=packet(1000,1,0); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
        s=packet(3000,2,1);
        if (clock==0) s.state_received_ms=1000;
        else if (clock==1) s.world_observed_ms=1000;
        else s.state_received_ms=s.world_observed_ms=s.mailbox_published_ms=1000;
        CHECK(!CoopRemoteTimelineAt(&t,&s,3000,&f) && !f.authority.ready);
        CHECK(diagnostics(&t).last_refusal_reason==COOP_TIMELINE_REFUSAL_EXPIRED);
        CHECK(diagnostics(&t).delivery.accepted_snapshots==0);
    }
    return 1;
}
static int wrap_reordering_and_mutation(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    CoopRemoteSnapshot s=packet(1000,UINT32_MAX,0); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=packet(1050,0,1); CHECK(CoopRemoteTimelineAt(&t,&s,1050,&f));
    CoopRemoteSnapshot old=packet(1060,UINT32_MAX,0);
    CHECK(!CoopRemoteTimelineAt(&t,&old,1060,&f)); CHECK(!CoopRemoteTimelineAt(&t,&old,1061,&f));
    CHECK(CoopRemoteTimelineAt(&t,&s,1062,&f));
    old=s; old.heading_valid=1; old.heading=1;
    CHECK(!CoopRemoteTimelineAt(&t,&old,1063,&f)); CHECK(!CoopRemoteTimelineAt(&t,&old,1064,&f));
    CHECK(diagnostics(&t).delivery.accepted_snapshots==2);
    CHECK(diagnostics(&t).refusals[COOP_TIMELINE_REFUSAL_SEQUENCE]==2);
    CHECK(diagnostics(&t).refusals[COOP_TIMELINE_REFUSAL_DUPLICATE]==2);
    s=packet(1070,1,1.1f); CHECK(CoopRemoteTimelineAt(&t,&s,1070,&f));
    return 1;
}
static int capacity_preserves_adaptive_anchor(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    CoopRemoteSnapshot s=packet(1000,1,0); CHECK(CoopRemoteTimelineAt(&t,&s,1000,&f));
    for (uint32_t i=1;i<=40;++i) {
        s=packet(1000+i,i+1,(float)i*0.001f);
        CHECK(CoopRemoteTimelineAt(&t,&s,s.state_received_ms,&f));
        CHECK(f.presentation_ms==1000 && near(f.position[0],0));
        CHECK(diagnostics(&t).snapshot_count<=COOP_TIMELINE_CAPACITY);
    }
    CHECK(diagnostics(&t).delivery.history_evictions>0);
    return 1;
}
static int overflow_keeps_current_curve(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CHECK(adaptive(&t));
    const float x[]={0,1,2,1,1,1,1,1}; CoopRemoteSnapshot s={0};
    for (uint32_t i=0;i<8;++i) {
        s=packet(1000+i*10,i+1,x[i]);
        CHECK(CoopRemoteTimelineAt(&t,&s,s.state_received_ms,&f));
    }
    CHECK(CoopRemoteTimelineAt(&t,&s,1115,&f) && near(f.position[0],1.5f));
    s=packet(1116,9,1); CHECK(CoopRemoteTimelineAt(&t,&s,1116,&f) && near(f.position[0],1.6f));
    s=packet(1117,10,1); CHECK(CoopRemoteTimelineAt(&t,&s,1117,&f) && near(f.position[0],1.7f));
    return 1;
}
int main(void) {
    const struct {const char *name; int (*run)(void);} cases[]={
        {"configuration and explicit reset",configuration}, {"stable 20Hz",stable_20hz},
        {"repeated snapshot and read-only diagnostics",repeat_and_readonly},
        {"same-ms bursts and hysteresis",bursts_and_hysteresis},
        {"sustained jitter, recovery, independent instances",jitter_recovery_and_instances},
        {"isolated anomaly after warm-up",isolated_anomaly_after_warmup},
        {"adaptive delay never rewinds presentation",render_clock_is_monotonic},
        {"underflow, gap and return burst",underflow_gap_and_burst_return},
        {"stop and restart",stop_and_restart}, {"wrap angles, unknown angle, XY/Z jumps",angle_and_relocation},
        {"available-data latency bound after underflow",available_latency_bound},
        {"six scope components and withdrawal",scope_and_withdrawal},
        {"three independent freshness clocks",independent_expiry},
        {"sequence wrap, stale and mutated duplicates",wrap_reordering_and_mutation},
        {"adaptive history overflow",capacity_preserves_adaptive_anchor},
        {"overflow preserves the currently displayed curve",overflow_keeps_current_curve}
    };
    unsigned int failed=0;
    for (unsigned int i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        int ok=cases[i].run(); printf("%s %s\n",ok ? "PASS" : "FAIL",cases[i].name); failed+=!ok;
    }
    return failed ? 1 : 0;
}
