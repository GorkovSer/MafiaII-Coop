/* Future trace replay only. Production algorithm is linked, not reimplemented.
   Synthetic source/receiver clocks share a known origin ONLY in this tool. */
#include "remote_timeline.h"
#include "wire.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {STABLE,JITTER,GAP_BURST,RECOVERY,STOP_TURN,RELOCATION,TRACE_COUNT};
static const char *const names[]={"stable","jitter","gap-burst","recovery","stop-turn","relocation"};
typedef struct {uint64_t at; CoopRemoteSnapshot snapshot;} Event;
typedef struct {
    uint64_t frames,available,holding_ms,hold_start,holds,unavailable;
    uint64_t rewinds,previous_presentation,previous_now,delivery_end,holding_during_delivery_ms;
    unsigned int have_previous,was_holding;
    uint64_t histogram[COOP_TIMELINE_DELAY_LIMIT_MS+1];
    uint64_t age_histogram[COOP_STATE_MAX_AGE_MS+1]; /* Last bucket is >=TTL. */
    double squared_error,max_error,angle_error;
    uint64_t angle_samples;
} Metrics;
static float truth_x(unsigned int kind,uint64_t at) {
    double t=at>1000 ? (double)(at-1000) : 0;
    double x=t<=3000 ? t*0.002 : t<=5000 ? 6 : 6+(t-5000)*0.002;
    if (kind==RELOCATION && t>=4000) x+=30;
    return (float)x;
}
static float truth_heading(uint64_t at) {
    const double pi=3.14159265358979323846;
    double t=at>1000 ? (double)(at-1000) : 0;
    return (float)(fmod(t*0.001+pi,2*pi)-pi);
}
static float angular_error(float a,float b) {
    return fabsf(atan2f(sinf(a-b),cosf(a-b)));
}
static unsigned int generate(unsigned int kind,Event *events) {
    unsigned int count=0,packets=kind==RECOVERY ? 720 : 200;
    uint64_t last_delivery=0;
    const unsigned int jitter[]={20,20,150,20,20,40,120,20};
    for (unsigned int i=0;i<packets;++i) {
        if (kind==GAP_BURST && i>=60 && i<=110) continue; /* Known synthetic omission. */
        uint64_t source=1000+(uint64_t)i*50;
        unsigned int delay=20;
        if (kind==JITTER || kind==GAP_BURST || (kind==RECOVERY && i<160)) delay=jitter[i%8];
        uint64_t received=source+delay;
        if (received<last_delivery) received=last_delivery; /* Queued catch-up bursts. */
        last_delivery=received;
        CoopRemoteSnapshot s={0};
        s.ready=s.active=s.have_state=s.world_authorized=1;
        s.session_id=7; s.player_id=2; s.epoch=s.world_generation=s.connection_generation=1;
        if (kind==GAP_BURST && i>110) {s.session_id=8; s.connection_generation=2;}
        s.received_sequence=i+1;
        s.state_received_ms=s.mailbox_published_ms=s.world_observed_ms=received;
        s.position[0]=truth_x(kind,source);
        s.heading_valid=!(kind==STOP_TURN && i%7==0);
        s.heading=s.heading_valid ? truth_heading(source) : 0;
        events[count++]=(Event){received,s};
    }
    return count;
}
static FILE *output(const char *prefix,const char *mode,const char *suffix) {
    char path[1024]; int n=snprintf(path,sizeof(path),"%s-%s-%s.csv",prefix,mode,suffix);
    if (n<0 || (size_t)n>=sizeof(path)) return NULL;
    return fopen(path,"w");
}
static int close_hold(FILE *holds,Metrics *m,uint64_t end) {
    if (!m->was_holding) return 1;
    m->holding_ms+=end-m->hold_start; ++m->holds;
    if(m->hold_start<m->delivery_end) {
        uint64_t limit=end<m->delivery_end ? end : m->delivery_end;
        m->holding_during_delivery_ms+=limit-m->hold_start;
    }
    int ok=fprintf(holds,"%llu,%llu,%llu\n",(unsigned long long)m->hold_start,
        (unsigned long long)end,(unsigned long long)(end-m->hold_start))>=0;
    m->was_holding=0; return ok;
}
static int replay(unsigned int kind,unsigned int adaptive_mode,const char *prefix,
    const Event *events,unsigned int count) {
    const char *mode=adaptive_mode ? "adaptive" : "fixed";
    FILE *frames=output(prefix,mode,"frames"),*holds=output(prefix,mode,"holds");
    FILE *summary=output(prefix,mode,"summary"),*delays=output(prefix,mode,"delays");
    if (!frames || !holds || !summary || !delays) {
        if(frames) fclose(frames);
        if(holds) fclose(holds);
        if(summary) fclose(summary);
        if(delays) fclose(delays);
        return 0;
    }
    fprintf(frames,"now_ms,available,sequence,presentation_ms,policy_delay_ms,effective_delay_ms,warming,holding,x,heading_valid,heading,truth_x,position_error,heading_error,reset_reason,refusal_reason,scope_latency_clamps\n");
    fprintf(holds,"begin_ms,end_ms,duration_ms\n");
    CoopRemoteTimeline t={0}; CoopRemoteFrame f; CoopRemoteSnapshot current={0}; Metrics m={0};
    m.delivery_end=events[count-1].at;
    if (adaptive_mode) {
        CoopTimelineConfig c; CoopRemoteTimelineDefaultConfig(&c); c.mode=COOP_INTERPOLATION_ADAPTIVE;
        if (!CoopRemoteTimelineConfigure(&t,&c)) {fclose(frames);fclose(holds);fclose(summary);fclose(delays);return 0;}
    }
    unsigned int next=0; uint64_t end=events[count-1].at+2100;
    for (uint64_t now=events[0].at;now<=end;now+=5) {
        while (next<count && events[next].at<=now) {
            current=events[next++].snapshot;
            /* Explicit mailbox observation for every delivery, including each
               same-ms event. Real consumers can coalesce events; see handoff. */
            CoopRemoteTimelineAt(&t,&current,current.state_received_ms,&f);
        }
        int available=CoopRemoteTimelineAt(&t,&current,now,&f);
        CoopTimelineDiagnostics d; CoopRemoteTimelineReadDiagnostics(&t,&d);
        ++m.frames;
        if (available) {
            ++m.available;
            if (d.delay_ms<=COOP_TIMELINE_DELAY_LIMIT_MS) ++m.histogram[d.delay_ms];
            unsigned int age=d.effective_delay_ms;
            if(age>COOP_STATE_MAX_AGE_MS) age=COOP_STATE_MAX_AGE_MS;
            ++m.age_histogram[age];
            double error=fabs((double)f.position[0]-truth_x(kind,f.presentation_ms));
            m.squared_error+=error*error; if(error>m.max_error)m.max_error=error;
            if(f.heading_valid){m.angle_error+=angular_error(f.heading,truth_heading(f.presentation_ms));++m.angle_samples;}
            if(m.have_previous && f.presentation_ms<m.previous_presentation) ++m.rewinds;
            m.previous_presentation=f.presentation_ms; m.have_previous=1;
            if (d.holding && !m.was_holding) {m.hold_start=now;m.was_holding=1;}
            else if (!d.holding) close_hold(holds,&m,now);
            fprintf(frames,"%llu,1,%u,%llu,%u,%u,%u,%u,%.7f,%u,%.7f,%.7f,%.7f,%.7f,%u,%u,%llu\n",
                (unsigned long long)now,f.authority.received_sequence,(unsigned long long)f.presentation_ms,
                d.delay_ms,d.effective_delay_ms,d.warming,d.holding,f.position[0],f.heading_valid,f.heading,
                truth_x(kind,f.presentation_ms),error,f.heading_valid ? angular_error(f.heading,truth_heading(f.presentation_ms)) : 0,
                d.last_reset_reason,d.last_refusal_reason,(unsigned long long)d.delivery.latency_clamps);
        } else {
            ++m.unavailable; m.have_previous=0; close_hold(holds,&m,now);
            fprintf(frames,"%llu,0,0,0,%u,0,%u,0,0,0,0,0,0,0,%u,%u,%llu\n",
                (unsigned long long)now,d.delay_ms,d.warming,d.last_reset_reason,d.last_refusal_reason,
                (unsigned long long)d.delivery.latency_clamps);
        }
        m.previous_now=now;
    }
    close_hold(holds,&m,m.previous_now+5);
    CoopTimelineDiagnostics d; CoopRemoteTimelineReadDiagnostics(&t,&d);
    fprintf(delays,"delay_ms,available_policy_frames,available_effective_age_frames\n");
    for(unsigned int i=0;i<=COOP_STATE_MAX_AGE_MS;++i) {
        uint64_t policy=i<=COOP_TIMELINE_DELAY_LIMIT_MS ? m.histogram[i] : 0;
        if(policy || m.age_histogram[i]) fprintf(delays,"%u,%llu,%llu\n",i,
            (unsigned long long)policy,(unsigned long long)m.age_histogram[i]);
    }
    fprintf(summary,"metric,value\ntrace,%s\nmode,%s\nrender_frames,%llu\navailable_frames,%llu\nunavailable_frames,%llu\nhold_intervals,%llu\nholding_ms,%llu\npresentation_rewinds_without_withdrawal,%llu\n",
        names[kind],mode,(unsigned long long)m.frames,(unsigned long long)m.available,
        (unsigned long long)m.unavailable,(unsigned long long)m.holds,(unsigned long long)m.holding_ms,
        (unsigned long long)m.rewinds);
    if(m.available) fprintf(summary,"position_rmse,%.9f\nposition_max_error,%.9f\n",sqrt(m.squared_error/m.available),m.max_error);
    else fprintf(summary,"position_rmse,unavailable\nposition_max_error,unavailable\n");
    if(m.angle_samples) fprintf(summary,"heading_mean_absolute_error,%.9f\n",m.angle_error/m.angle_samples);
    else fprintf(summary,"heading_mean_absolute_error,unavailable\n");
    fprintf(summary,"last_delivery_ms,%llu\nholding_during_delivery_ms,%llu\ntail_holding_ms,%llu\n",
        (unsigned long long)m.delivery_end,(unsigned long long)m.holding_during_delivery_ms,
        (unsigned long long)(m.holding_ms-m.holding_during_delivery_ms));
    for(unsigned int i=1;i<COOP_TIMELINE_RESET_REASON_COUNT;++i)
        fprintf(summary,"reset_reason_%u,%llu\n",i,(unsigned long long)d.resets[i]);
    for(unsigned int i=1;i<COOP_TIMELINE_REFUSAL_REASON_COUNT;++i)
        fprintf(summary,"refusal_reason_%u,%llu\n",i,(unsigned long long)d.refusals[i]);
    int ok=!ferror(frames) && !ferror(holds) && !ferror(summary) && !ferror(delays);
    if(fclose(frames))ok=0;
    if(fclose(holds))ok=0;
    if(fclose(summary))ok=0;
    if(fclose(delays))ok=0;
    return ok;
}
int main(int argc,char **argv) {
    if(argc!=3){fprintf(stderr,"Usage: interpolation_trace stable|jitter|gap-burst|recovery|stop-turn|relocation OUTPUT_PREFIX\n");return 2;}
    unsigned int kind;
    for(kind=0;kind<TRACE_COUNT;++kind)if(!strcmp(argv[1],names[kind]))break;
    if(kind==TRACE_COUNT)return 2;
    Event *events=calloc(720,sizeof(*events)); if(!events)return 3;
    unsigned int count=generate(kind,events);
    int ok=replay(kind,0,argv[2],events,count) && replay(kind,1,argv[2],events,count);
    free(events); return ok ? 0 : 1;
}
