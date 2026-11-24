#include "game_bridge.h"
#include "wire.h"
#include <math.h>
#include <string.h>

int CoopRemoteSnapshotAt(const CoopRemoteSnapshot *source, uint64_t now, CoopRemoteSnapshot *out) {
    if (!out) return 0;
    CoopRemoteSnapshot candidate = {0};
    if (source) candidate = *source; /* Also permit source == out. */
    memset(out, 0, sizeof(*out));
    if (!source || !candidate.ready || !candidate.session_id || !candidate.connection_generation ||
        now < candidate.mailbox_published_ms || now - candidate.mailbox_published_ms >= COOP_STATE_MAX_AGE_MS)
        return 0;
    *out = candidate;
    int usable = candidate.active == 1 && candidate.have_state && candidate.epoch &&
        (candidate.player_id == 1 || candidate.player_id == 2) &&
        candidate.state_received_ms <= candidate.mailbox_published_ms &&
        candidate.state_received_ms <= now && now - candidate.state_received_ms < COOP_STATE_MAX_AGE_MS;
    for (unsigned int i = 0; usable && i < 3; ++i)
        usable = isfinite(candidate.position[i]) && fabsf(candidate.position[i]) < 1000000.0f;
    usable = usable && CoopHeadingValid(candidate.heading, candidate.heading_valid);
    out->active = (uint32_t)usable;
    out->world_authorized = (uint32_t)(usable && candidate.world_authorized == 1 &&
        candidate.world_generation && candidate.world_observed_ms <= candidate.mailbox_published_ms &&
        candidate.world_observed_ms <= now && now - candidate.world_observed_ms < COOP_STATE_MAX_AGE_MS);
    if (!usable) {
        memset(out->position, 0, sizeof(out->position));
        out->heading = 0; out->heading_valid = 0;
    }
    return 1;
}
