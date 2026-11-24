/* Pure clock/validity cases; does not start ENet or game processes. */
#include "../src/net/game_bridge.h"
#include "../src/net/session.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CoopRemoteSnapshot fresh(void) {
    CoopRemoteSnapshot s = {0};
    s.ready = s.active = s.have_state = 1; s.epoch = 1; s.player_id = 2;
    s.connection_generation = 1; s.session_id = 7;
    s.mailbox_published_ms = s.state_received_ms = 10000;
    s.position[0] = 4; s.position[1] = -2; s.position[2] = 1;
    return s;
}
static void check(const CoopRemoteSnapshot *source, uint64_t now, int ready, int active) {
    CoopRemoteSnapshot out, zero = {0}; memset(&out, 0xaa, sizeof(out));
    int result = CoopRemoteSnapshotAt(source, now, &out);
    if (result != ready || out.active != (uint32_t)active ||
        (!ready && memcmp(&out, &zero, sizeof(out))) ||
        (!active && (out.position[0] || out.position[1] || out.position[2] ||
            out.heading || out.heading_valid))) {
        fputs("remote validity assertion failed\n", stderr); exit(1);
    }
}
int main(void) {
    CoopRemoteSnapshot s = fresh();
    s.heading_valid=1; s.heading=1.25f;
    check(&s,10000,1,1);
    CoopRemoteSnapshot pose;
    if (!CoopRemoteSnapshotAt(&s,10000,&pose) || !pose.heading_valid || pose.heading!=1.25f) return 1;
    s.heading=NAN; check(&s,10000,1,0);
    s.heading=4; check(&s,10000,1,0);
    s.heading=0; s.heading_valid=2; check(&s,10000,1,0);
    s.heading=1; s.heading_valid=0; check(&s,10000,1,0);
    s=fresh();
    check(&s, 10000, 1, 1); check(&s, 11999, 1, 1);
    check(&s, 12000, 0, 0); /* worker stopped updating */
    s.mailbox_published_ms = 11999;
    check(&s, 11999, 1, 1); check(&s, 12000, 1, 0); /* worker live, state expired */
    s.state_received_ms = 12000; s.mailbox_published_ms = 12000;
    check(&s, 12000, 1, 1); /* only new accepted state restores activity */
    s.active = 0; check(&s, 12000, 1, 0);
    s = fresh(); s.have_state = 0; check(&s, 10000, 1, 0);
    s = fresh(); s.ready = 0; check(&s, 10000, 0, 0);
    s = fresh(); s.session_id = 0; check(&s, 10000, 0, 0);
    s = fresh(); s.connection_generation = 0; check(&s, 10000, 0, 0);
    s = fresh(); s.epoch = 0; check(&s, 10000, 1, 0);
    s = fresh(); s.player_id = 3; check(&s, 10000, 1, 0);
    s = fresh(); s.position[0] = NAN; check(&s, 10000, 1, 0);
    s = fresh(); s.position[1] = 1000000.0f; check(&s, 10000, 1, 0);
    s = fresh(); check(&s, 9999, 0, 0);
    s.state_received_ms = 10001; check(&s, 10002, 1, 0); /* impossible timestamp ordering */
    check(NULL, 10000, 0, 0);
    s = fresh(); if (!CoopRemoteSnapshotAt(&s, 10000, &s) || !s.active) return 1;
    if (CoopRemoteSnapshotAt(&s, 10000, NULL)) return 1;
    s = fresh(); s.world_authorized = 1; s.world_generation = 8; s.world_observed_ms = 10000;
    CoopRemoteSnapshot world;
    if (!CoopRemoteSnapshotAt(&s, 10000, &world) || !world.world_authorized) return 1;
    s.mailbox_published_ms = s.state_received_ms = 12000;
    if (!CoopRemoteSnapshotAt(&s, 12000, &world) || world.world_authorized) return 1;
    s.world_observed_ms = 12001;
    if (!CoopRemoteSnapshotAt(&s, 12000, &world) || world.world_authorized) return 1;
    s.world_observed_ms = 12000; s.world_generation = 0;
    if (!CoopRemoteSnapshotAt(&s, 12000, &world) || world.world_authorized) return 1;
    CoopSession clock = {0};
    if (CoopSessionRemoteAge(&clock, 0) != UINT32_MAX) return 1;
    clock.have_sequence = 1; clock.remote_received_at = UINT32_MAX - 999;
    if (CoopSessionRemoteAge(&clock, 999) != 1999 || CoopSessionRemoteAge(&clock, 1000) != 2000) return 1;
    puts("PASS receiver validity: state/worker expiry, inactive clearing, recovery, clock wrap and invalid metadata");
    return 0;
}
