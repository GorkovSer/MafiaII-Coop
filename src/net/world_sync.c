#include "world_sync.h"
#include <string.h>

int CoopWorldKeyValid(const unsigned char key[32]) {
    unsigned char any = 0;
    if (!key) return 0;
    for (unsigned int i = 0; i < 32; ++i) any |= key[i];
    return any != 0;
}
void CoopWorldInit(CoopWorldSync *s, uint32_t role) {
    memset(s, 0, sizeof(*s)); s->role = role;
}
static void pending(CoopWorldSync *s, uint32_t type, uint32_t epoch, const unsigned char *key) {
    memset(&s->pending, 0, sizeof(s->pending));
    s->pending.generation = s->offer.generation; s->pending.epoch = epoch;
    if (key) memcpy(s->pending.key, key, 32);
    s->pending_type = type;
}
static void guest_ack(CoopWorldSync *s) {
    if (!s->offer.generation) return;
    int match = s->local_epoch && CoopWorldKeyValid(s->offer.key) &&
        !memcmp(s->local_key, s->offer.key, 32);
    pending(s, COOP_WORLD_ACK, match ? s->local_epoch : 0, match ? s->local_key : NULL);
}
void CoopWorldSetLocal(CoopWorldSync *s, uint32_t epoch, const unsigned char *key) {
    unsigned char empty[32] = {0};
    if (!epoch || !CoopWorldKeyValid(key)) { epoch = 0; key = empty; }
    if (epoch == s->local_epoch && !memcmp(key, s->local_key, 32)) return;
    s->local_epoch = epoch; memcpy(s->local_key, key, 32);
    s->ready = s->remote_epoch = 0;
    if (s->role == 1) {
        /* Never reuse a generation within a connection. Restart the session
           on exhaustion rather than allow an old world to become current. */
        if (s->next_generation == UINT32_MAX) {
            s->exhausted = 1; s->pending_type = 0; return;
        }
        memset(&s->offer, 0, sizeof(s->offer));
        s->offer.generation = ++s->next_generation;
        s->offer.epoch = epoch; memcpy(s->offer.key, key, 32);
        pending(s, COOP_WORLD_OFFER, epoch, key);
    } else if (s->role == 2) guest_ack(s);
}
int CoopWorldReceive(CoopWorldSync *s, uint32_t type, const CoopWorldToken *t) {
    if (!s || !t || !t->generation || s->exhausted) return 0;
    int populated = CoopWorldKeyValid(t->key);
    if (populated != (t->epoch != 0)) return 0;
    if (s->role == 2 && type == COOP_WORLD_OFFER) {
        if (t->generation < s->offer.generation) return 1; /* stale, no mutation */
        if (t->generation == s->offer.generation) {
            if (memcmp(t, &s->offer, sizeof(*t))) return 0;
            guest_ack(s); return 1;
        }
        s->offer = *t; s->ready = 0; s->remote_epoch = 0;
        guest_ack(s); return 1;
    }
    if (s->role == 1 && type == COOP_WORLD_ACK) {
        if (t->generation < s->offer.generation) return 1;
        if (t->generation != s->offer.generation) return 0;
        s->ready = 0; s->remote_epoch = 0; s->pending_type = 0;
        if (!populated) return 1; /* guest unloaded or has another world */
        if (!s->local_epoch || memcmp(t->key, s->local_key, 32)) return 0;
        s->remote_epoch = t->epoch;
        /* Readiness begins only after COMMIT was successfully queued. */
        pending(s, COOP_WORLD_COMMIT, t->epoch, t->key); return 1;
    }
    if (s->role == 2 && type == COOP_WORLD_COMMIT) {
        if (t->generation < s->offer.generation) return 1;
        if (t->generation == s->offer.generation &&
            (!s->local_epoch || t->epoch != s->local_epoch || memcmp(s->local_key, s->offer.key, 32)))
            return 1; /* local load changed while the acknowledgement was in flight */
        if (t->generation != s->offer.generation || !populated ||
            t->epoch != s->local_epoch || memcmp(t->key, s->local_key, 32) ||
            memcmp(t->key, s->offer.key, 32)) return 0;
        s->remote_epoch = s->offer.epoch; s->ready = 1; return 1;
    }
    return 0;
}
void CoopWorldSent(CoopWorldSync *s) {
    if (s->pending_type == COOP_WORLD_COMMIT) s->ready = 1;
    s->pending_type = 0;
}
int CoopWorldAccepts(const CoopWorldSync *s, uint32_t generation, uint32_t epoch) {
    return s && !s->exhausted && s->ready && s->local_epoch &&
        generation && generation == s->offer.generation && epoch && epoch == s->remote_epoch;
}
