#ifndef COOP_HUMAN_AI_H
#define COOP_HUMAN_AI_H
#include <stddef.h>
#include <stdint.h>
typedef int (*CoopAiRead)(void *context, uint32_t address, void *out, size_t size);
typedef enum {
    COOP_AI_UNREADABLE, COOP_AI_LAYOUT_MISMATCH, COOP_AI_CHANGED, COOP_AI_VALID
} CoopHumanAiStatus;
typedef struct {
    uint32_t controller, agent;
    uint8_t mode_flags; /* Raw controller+0x170, NOT an AI-disabled verdict. */
} CoopHumanAi;
/* Caller must establish pinned image, game thread and current registry identity.
   Reads only. Valid layout is NOT permission to change AI or move the Human.
   Output is zero on refusal; no borrowed engine pointer acquires ownership. */
CoopHumanAiStatus CoopReadHumanAi(CoopAiRead read, void *context,
    uint32_t human, uint32_t guid, CoopHumanAi *out);
#endif
