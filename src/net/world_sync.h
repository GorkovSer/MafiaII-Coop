#ifndef COOP_WORLD_SYNC_H
#define COOP_WORLD_SYNC_H
#include <stdint.h>

/* Caller supplies a verified loaded-world digest, never a save request ID.
   Single network-thread owner. No engine calls, clocks or allocation. */
enum { COOP_WORLD_OFFER = 4, COOP_WORLD_ACK = 5, COOP_WORLD_COMMIT = 6 };
typedef struct {
    uint32_t generation, epoch;
    unsigned char key[32];
} CoopWorldToken;
typedef struct {
    uint32_t role, local_epoch, remote_epoch, ready, pending_type;
    uint32_t next_generation, exhausted;
    unsigned char local_key[32];
    CoopWorldToken offer, pending;
} CoopWorldSync;
int CoopWorldKeyValid(const unsigned char key[32]);
void CoopWorldInit(CoopWorldSync *sync, uint32_t role);
void CoopWorldSetLocal(CoopWorldSync *sync, uint32_t epoch, const unsigned char *verified_key);
int CoopWorldReceive(CoopWorldSync *sync, uint32_t type, const CoopWorldToken *token);
void CoopWorldSent(CoopWorldSync *sync); /* Only after reliable enqueue succeeds. */
int CoopWorldAccepts(const CoopWorldSync *sync, uint32_t generation, uint32_t remote_epoch);
#endif
