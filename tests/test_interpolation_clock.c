/* Code-only regression scenario. Not compiled or executed during preparation. */
#include "remote_timeline.h"
#include <assert.h>
#include <stdio.h>
static CoopRemoteSnapshot packet(uint64_t ms, uint32_t sequence, float x) {
    CoopRemoteSnapshot s={0};
    s.ready=s.active=s.have_state=s.world_authorized=1;
    s.session_id=1; s.connection_generation=s.world_generation=s.epoch=1; s.player_id=2;
    s.mailbox_published_ms=s.state_received_ms=s.world_observed_ms=ms;
    s.received_sequence=sequence; s.position[0]=x;
    return s;
}
int main(void) {
    CoopRemoteTimeline t={0}; CoopRemoteFrame f;
    CoopRemoteSnapshot s=packet(1000,10,1);
    assert(CoopRemoteTimelineAt(&t,&s,1000,&f));
    s=packet(1050,11,2);
    assert(CoopRemoteTimelineAt(&t,&s,1200,&f));
    /* Both fields roll back, although the snapshot is individually fresh.
       PR21 reset its watermark before comparing this candidate. */
    CoopRemoteSnapshot old=packet(1010,10,1);
    assert(!CoopRemoteTimelineAt(&t,&old,1100,&f) && !f.authority.ready);
    assert(!CoopRemoteTimelineAt(&t,&old,1150,&f) && !f.authority.ready);
    assert(!CoopRemoteTimelineAt(&t,&old,1200,&f) && !f.authority.ready);
    assert(CoopRemoteTimelineAt(&t,&s,1201,&f) && f.position[0]==2);
    puts("PASS receiver clock rollback cannot reset packet identity");
    return 0;
}
