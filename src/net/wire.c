#include "wire.h"
#include <math.h>
#include <string.h>

const unsigned char CoopGameBuild[32] = {
    0x16,0xc7,0x9a,0x3b,0xe4,0x83,0xe5,0x82,0x71,0x9e,0x43,0xd7,0xb4,0x51,0xe8,0x87,
    0xe8,0x94,0x14,0xbe,0x81,0xca,0x7e,0xcf,0xc0,0x53,0x6d,0xd8,0x18,0x5e,0x6c,0x2f};

static void put32(unsigned char *at, uint32_t value) {
    for (unsigned int i = 0; i < 4; ++i) at[i] = (unsigned char)(value >> (24 - i*8));
}
static uint32_t get32(const unsigned char *at) {
    return ((uint32_t)at[0] << 24) | ((uint32_t)at[1] << 16) | ((uint32_t)at[2] << 8) | at[3];
}
static size_t payload_size(unsigned int type) {
    switch (type) {
        case COOP_HELLO: return 40; case COOP_WELCOME: return 12; case COOP_STATE: return 36;
        case COOP_WORLD_OFFER: case COOP_WORLD_ACK: case COOP_WORLD_COMMIT: return 40;
        default: return 0;
    }
}
int CoopHeadingValid(float heading, uint32_t available) {
    return available <= 1 && isfinite(heading) && fabsf(heading) <= COOP_HEADING_MAX &&
        (available || heading == 0.0f);
}
static int valid(const CoopMessage *m) {
    if (m->type == COOP_STATE) {
        if (m->active > 1 || (m->player_id != 1 && m->player_id != 2)) return 0;
        for (unsigned int i = 0; i < 3; ++i)
            if (!isfinite(m->position[i]) || fabsf(m->position[i]) >= 1000000.0f ||
                (!m->active && m->position[i] != 0.0f)) return 0;
        if (m->active && !m->epoch) return 0;
        if (!CoopHeadingValid(m->heading, m->heading_valid) ||
            (!m->active && m->heading_valid)) return 0;
    }
    if (m->type >= COOP_WORLD_OFFER && m->type <= COOP_WORLD_COMMIT) {
        if (!m->world.generation || CoopWorldKeyValid(m->world.key) != (m->world.epoch != 0)) return 0;
        if (m->type == COOP_WORLD_COMMIT && !m->world.epoch) return 0;
    }
    return payload_size(m->type) != 0;
}
int CoopSequenceNewer(uint32_t value, uint32_t previous) {
    uint32_t delta = value - previous;
    return delta != 0 && delta < 0x80000000u;
}
size_t CoopWireEncode(const CoopMessage *m, unsigned char out[COOP_WIRE_MAX]) {
    _Static_assert(sizeof(float) == 4, "32-bit float required");
    size_t payload = payload_size(m->type);
    if (!valid(m)) return 0;
    memset(out, 0, COOP_WIRE_MAX);
    memcpy(out, "MIIC", 4);
    out[5] = COOP_PROTOCOL; out[7] = (unsigned char)m->type;
    put32(out+8, (uint32_t)(m->session >> 32)); put32(out+12, (uint32_t)m->session);
    put32(out+16, m->sequence); out[21] = (unsigned char)payload;
    if (m->type == COOP_HELLO) { memcpy(out+24, m->build, 32); put32(out+56, m->capabilities); put32(out+60,m->module_version); }
    if (m->type == COOP_WELCOME) { put32(out+24, m->player_id); put32(out+28, m->capabilities); put32(out+32,m->module_version); }
    if (m->type == COOP_STATE) {
        put32(out+24, m->player_id); put32(out+28, m->epoch); put32(out+32, m->active);
        for (unsigned int i = 0; i < 3; ++i) { uint32_t bits; memcpy(&bits, &m->position[i], 4); put32(out+36+i*4, bits); }
        put32(out+48, m->world.generation);
        uint32_t heading_bits; memcpy(&heading_bits, &m->heading, 4);
        put32(out+52, heading_bits); put32(out+56, m->heading_valid);
    }
    if (m->type >= COOP_WORLD_OFFER && m->type <= COOP_WORLD_COMMIT) {
        put32(out+24, m->world.generation); put32(out+28, m->world.epoch);
        memcpy(out+32, m->world.key, 32);
    }
    return payload + 24;
}
int CoopWireDecode(const unsigned char *bytes, size_t length, CoopMessage *out) {
    if (length < 24 || length > COOP_WIRE_MAX || memcmp(bytes, "MIIC", 4) || bytes[4] ||
        bytes[5] != COOP_PROTOCOL || bytes[6] || bytes[20] || bytes[22] || bytes[23]) return 0;
    size_t payload = payload_size(bytes[7]);
    if (!payload || bytes[21] != payload || length != payload + 24) return 0;
    memset(out, 0, sizeof(*out));
    out->type = bytes[7]; out->session = ((uint64_t)get32(bytes+8) << 32) | get32(bytes+12);
    out->sequence = get32(bytes+16);
    if (out->type == COOP_HELLO) { memcpy(out->build, bytes+24, 32); out->capabilities = get32(bytes+56); out->module_version=get32(bytes+60); }
    if (out->type == COOP_WELCOME) { out->player_id = get32(bytes+24); out->capabilities = get32(bytes+28); out->module_version=get32(bytes+32); }
    if (out->type == COOP_STATE) {
        out->player_id = get32(bytes+24); out->epoch = get32(bytes+28); out->active = get32(bytes+32);
        for (unsigned int i = 0; i < 3; ++i) { uint32_t bits = get32(bytes+36+i*4); memcpy(&out->position[i], &bits, 4); }
        out->world.generation = get32(bytes+48);
        uint32_t heading_bits = get32(bytes+52);
        memcpy(&out->heading, &heading_bits, 4); out->heading_valid = get32(bytes+56);
    }
    if (out->type >= COOP_WORLD_OFFER && out->type <= COOP_WORLD_COMMIT) {
        out->world.generation = get32(bytes+24); out->world.epoch = get32(bytes+28);
        memcpy(out->world.key, bytes+32, 32);
    }
    return valid(out);
}
