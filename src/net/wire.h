#ifndef COOP_WIRE_H
#define COOP_WIRE_H
#include <stddef.h>
#include <stdint.h>

#define COOP_PROTOCOL 1
#define COOP_MODULE_VERSION 3u /* diagnostic 0.0.3; exact compatibility */
#define COOP_CAP_TELEMETRY 1u
#define COOP_WIRE_MAX 64
enum { COOP_HELLO = 1, COOP_WELCOME = 2, COOP_STATE = 3 };
typedef struct {
    uint16_t type;
    uint64_t session;
    uint32_t sequence;
    uint32_t player_id, epoch, active;
    float position[3];
    unsigned char build[32];
    uint32_t capabilities;
    uint32_t module_version;
} CoopMessage;

extern const unsigned char CoopGameBuild[32];
size_t CoopWireEncode(const CoopMessage *message, unsigned char out[COOP_WIRE_MAX]);
int CoopWireDecode(const unsigned char *bytes, size_t length, CoopMessage *out);
int CoopSequenceNewer(uint32_t value, uint32_t previous);
#endif
