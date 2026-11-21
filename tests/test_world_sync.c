/* Authored for deferred execution; this is not two-client/game acceptance. */
#include "../src/net/world_sync.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void deliver(CoopWorldSync *from, CoopWorldSync *to) {
    assert(from->pending_type);
    assert(CoopWorldReceive(to, from->pending_type, &from->pending));
    CoopWorldSent(from);
}
int main(void) {
    CoopWorldSync host, guest;
    unsigned char key[32] = {1}, other[32] = {2}, zero[32] = {0};
    CoopWorldInit(&host, 1); CoopWorldInit(&guest, 2);
    assert(!CoopWorldKeyValid(NULL) && !CoopWorldKeyValid(zero));
    assert(!CoopWorldAccepts(&host, 1, 1));
    CoopWorldSetLocal(&host, 10, key); CoopWorldSetLocal(&guest, 90, key);
    assert(host.pending_type == COOP_WORLD_OFFER && !guest.pending_type);
    CoopWorldToken first = host.offer;
    deliver(&host, &guest); assert(!host.ready && !guest.ready);
    assert(guest.pending.epoch == 90);
    deliver(&guest, &host); assert(!host.ready && !guest.ready);
    assert(host.pending_type == COOP_WORLD_COMMIT);
    deliver(&host, &guest);
    assert(CoopWorldAccepts(&host, first.generation, 90));
    assert(CoopWorldAccepts(&guest, first.generation, 10));
    assert(!CoopWorldAccepts(&host, first.generation, 10));
    assert(!CoopWorldAccepts(&guest, 0, 10));
    CoopWorldSetLocal(&host, 10, key); assert(!host.pending_type && host.ready);

    /* Guest loses authority immediately; withdrawal reaches the host. */
    CoopWorldSetLocal(&guest, 0, NULL); assert(!guest.ready);
    deliver(&guest, &host); assert(!host.ready);
    CoopWorldSetLocal(&guest, 91, key); deliver(&guest, &host);
    CoopWorldToken delayed = host.pending;
    CoopWorldSent(&host);
    CoopWorldSetLocal(&guest, 92, key);
    assert(CoopWorldReceive(&guest, COOP_WORLD_COMMIT, &delayed) && !guest.ready);
    deliver(&guest, &host); deliver(&host, &guest);
    assert(CoopWorldAccepts(&host, first.generation, 92));
    assert(!CoopWorldAccepts(&host, first.generation, 91));

    /* Host rewind, including the same world key, gets a new generation. */
    CoopWorldSetLocal(&host, 11, key); assert(!host.ready);
    deliver(&host, &guest); assert(!guest.ready);
    assert(!CoopWorldAccepts(&guest, first.generation, 10));
    assert(CoopWorldReceive(&guest, COOP_WORLD_OFFER, &first));
    assert(guest.offer.generation > first.generation);
    deliver(&guest, &host); deliver(&host, &guest);
    assert(host.ready && guest.ready);

    /* Different loaded world never acknowledges readiness. */
    CoopWorldSetLocal(&guest, 93, other); deliver(&guest, &host);
    assert(!host.ready && !guest.ready);
    CoopWorldSetLocal(&host, 0, NULL); deliver(&host, &guest);
    assert(!guest.ready && !CoopWorldKeyValid(guest.offer.key));
    deliver(&guest, &host); assert(!host.ready);

    /* Malformed/incorrect-role controls and generation exhaustion. */
    CoopWorldToken bad = {0}; bad.generation = 99; bad.epoch = 1;
    assert(!CoopWorldReceive(&guest, COOP_WORLD_OFFER, &bad));
    bad.key[0] = 1;
    assert(!CoopWorldReceive(&host, COOP_WORLD_OFFER, &bad));
    assert(!CoopWorldReceive(&guest, COOP_WORLD_ACK, &bad));
    assert(!CoopWorldReceive(&guest, COOP_WORLD_COMMIT, &bad));
    host.next_generation = UINT32_MAX;
    CoopWorldSetLocal(&host, 12, key);
    assert(host.exhausted && !host.ready && !host.pending_type);
    CoopWorldInit(&host, 1); assert(!host.ready && !host.offer.generation);
    puts("PASS world offer/ack/commit, withdrawal, load races and generation rejection");
    return 0;
}
