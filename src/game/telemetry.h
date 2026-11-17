#ifndef COOP_TELEMETRY_H
#define COOP_TELEMETRY_H
#include <windows.h>
#include <stdint.h>

/* Local entity GUID is an observation, not a network identity. No pointers
   are retained between ticks or written to the snapshot. */
typedef struct {
    uint32_t epoch;
    uint32_t local_guid;
    float position[3];
    float model_position[3];
    int model_valid;
} CoopPlayerSnapshot;

int CoopTelemetryStart(const wchar_t *log_path);
void CoopTelemetryEvent(unsigned int event_id);
#endif
