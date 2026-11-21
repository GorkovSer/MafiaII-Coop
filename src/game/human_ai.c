#include "human_ai.h"
#include <string.h>
static int take(CoopAiRead read, void *context, uint32_t base, uint32_t offset,
    void *out, size_t size) {
    uint64_t at = (uint64_t)base + offset;
    return base && at <= UINT32_MAX && size <= UINT64_C(0x100000000) - at &&
        read(context, (uint32_t)at, out, size);
}
static uint32_t word(const unsigned char *p) {
    uint32_t v; memcpy(&v, p, sizeof(v)); return v;
}
CoopHumanAiStatus CoopReadHumanAi(CoopAiRead read, void *context,
    uint32_t human, uint32_t guid, CoopHumanAi *out) {
    if (!out) return COOP_AI_UNREADABLE;
    memset(out, 0, sizeof(*out));
    if (!read || !human || !guid || (guid & 255) != 0x0e) return COOP_AI_LAYOUT_MISMATCH;
    unsigned char h[0xac], a[0x171], h_again[0xac], a_again[0x171];
    if (!take(read, context, human, 0, h, sizeof(h))) return COOP_AI_UNREADABLE;
    if (word(h) != 0x1967058 || word(h+0x1c) != guid || !word(h+0xa8))
        return COOP_AI_LAYOUT_MISMATCH;
    uint32_t controller = word(h+0xa8);
    if (!take(read, context, controller, 0, a, sizeof(a))) return COOP_AI_UNREADABLE;
    if (word(a) != 0x1967228 || word(a+8) != human || word(a+0xe4) != human)
        return COOP_AI_LAYOUT_MISMATCH;
    if (!take(read, context, human, 0, h_again, sizeof(h_again)) ||
        !take(read, context, controller, 0, a_again, sizeof(a_again))) return COOP_AI_UNREADABLE;
    /* Compare relevant identities/state only; unrelated component counters can
       legitimately change. Stable repeated bytes are not a lifetime guarantee. */
    if (word(h_again) != word(h) || word(h_again+0x1c) != guid ||
        word(h_again+0xa8) != controller || word(a_again) != word(a) ||
        word(a_again+8) != human || word(a_again+0xe4) != human ||
        word(a_again+0xc) != word(a+0xc) || a_again[0x170] != a[0x170])
        return COOP_AI_CHANGED;
    out->controller = controller; out->agent = word(a+0xc); out->mode_flags = a[0x170];
    return COOP_AI_VALID;
}
