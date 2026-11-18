/* Production reader on owned synthetic bytes only; never a game/world test. */
#include "../src/game/script_progress.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { BASE=0x1000, SLOT=0x1004, MANAGER=0x2000, POOL=MANAGER+0x4b8 };
static unsigned char memory[0x5000];
static unsigned int calls, record_reads, name_reads, slot_reads;
static int change_records, change_name, change_slot, fail_records;
static void put(uint32_t address, const void *data, size_t size) {
    memcpy(memory + address - BASE, data, size);
}
static void word(uint32_t address, uint32_t value) { put(address, &value, 4); }
static void entry(unsigned int index, uint32_t offset, float value, uint16_t type) {
    uint32_t address = MANAGER+4+index*12;
    put(address, &offset, 4); put(address+4, &value, 4); put(address+8, &type, 2);
}
static int read_memory(void *context, uint32_t address, void *out, size_t size) {
    (void)context;
    ++calls;
    if (calls > 1000 || size > 1200 || address < BASE ||
        (uint64_t)address + size > BASE + sizeof(memory)) return 0;
    if (address == MANAGER+4) {
        ++record_reads;
        if (fail_records) return 0;
    }
    memcpy(out, memory + address - BASE, size);
    if (address == MANAGER+4 && change_records && record_reads == 2)
        ((unsigned char *)out)[0] ^= 1;
    if (address == POOL && size == 14 && change_name && ++name_reads == 2)
        ((unsigned char *)out)[0] ^= 1;
    if (address == SLOT && change_slot && ++slot_reads == 2) {
        uint32_t changed = MANAGER+4;
        memcpy(out, &changed, 4);
    }
    return 1;
}
static void reset(void) {
    memset(memory, 0, sizeof(memory));
    calls = record_reads = name_reads = slot_reads = 0;
    change_records = change_name = change_slot = fail_records = 0;
    word(SLOT, MANAGER); word(MANAGER+0x4b4, 2);
    static const char names[] = "missionNumber\0missionPart";
    put(POOL, names, sizeof(names));
    entry(0, 0, 2, 2); entry(1, 14, 3, 2);
}
static int expect(CoopProgressStatus expected) {
    CoopScriptProgress result = {0xffffffffu, 0xffffffffu};
    calls = record_reads = name_reads = slot_reads = 0;
    CoopProgressStatus status = CoopReadScriptProgress(read_memory, NULL, SLOT, &result);
    if (status != expected) return 0;
    return expected == COOP_PROGRESS_OBSERVED || (!result.mission_number && !result.mission_part);
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); return 1; } } while (0)
int main(void) {
    reset(); CHECK(expect(COOP_PROGRESS_OBSERVED));
    CoopScriptProgress result;
    CHECK(CoopReadScriptProgress(read_memory, NULL, SLOT, &result) == COOP_PROGRESS_OBSERVED);
    CHECK(result.mission_number == 2 && result.mission_part == 3);
    word(MANAGER+0x4b4, 0); CHECK(expect(COOP_PROGRESS_MISSING));
    word(MANAGER+0x4b4, 101); CHECK(expect(COOP_PROGRESS_BAD_LAYOUT));
    reset(); word(MANAGER+0x4b4, 100);
    for (unsigned int i=0; i<100; ++i) entry(i, 0, 2, 2);
    memory[POOL-BASE] = 'x';
    CHECK(expect(COOP_PROGRESS_MISSING)); CHECK(calls <= 500);
    reset(); entry(0, 0, NAN, 2); CHECK(expect(COOP_PROGRESS_BAD_VALUE));
    entry(0, 0, INFINITY, 2); CHECK(expect(COOP_PROGRESS_BAD_VALUE));
    entry(0, 0, -1, 2); CHECK(expect(COOP_PROGRESS_BAD_VALUE));
    entry(0, 0, 2.5f, 2); CHECK(expect(COOP_PROGRESS_BAD_VALUE));
    entry(0, 0, 65536, 2); CHECK(expect(COOP_PROGRESS_BAD_VALUE));
    entry(0, 0, 2, 1); CHECK(expect(COOP_PROGRESS_BAD_VALUE));
    reset(); entry(1, 4096, 3, 2); CHECK(expect(COOP_PROGRESS_BAD_LAYOUT));
    reset(); memory[POOL-BASE] = 'x'; CHECK(expect(COOP_PROGRESS_MISSING));
    reset(); word(MANAGER+0x4b4, 3); entry(2, 0, 9, 2);
    CHECK(expect(COOP_PROGRESS_OBSERVED));
    CHECK(CoopReadScriptProgress(read_memory, NULL, SLOT, &result) == COOP_PROGRESS_OBSERVED);
    CHECK(result.mission_number == 9);
    entry(2, 0, 9, 1); CHECK(expect(COOP_PROGRESS_BAD_VALUE));
    reset(); change_records=1; CHECK(expect(COOP_PROGRESS_CHANGED));
    reset(); change_name=1; CHECK(expect(COOP_PROGRESS_CHANGED));
    reset(); change_slot=1; CHECK(expect(COOP_PROGRESS_CHANGED));
    reset(); fail_records=1; CHECK(expect(COOP_PROGRESS_UNREADABLE));
    reset(); word(SLOT, 0xfffffffc); CHECK(expect(COOP_PROGRESS_BAD_LAYOUT));
    word(SLOT, MANAGER+1); CHECK(expect(COOP_PROGRESS_BAD_LAYOUT));
    word(SLOT, 0); CHECK(expect(COOP_PROGRESS_UNREADABLE));
    memset(&result, 0xff, sizeof(result));
    CHECK(CoopReadScriptProgress(NULL, NULL, SLOT, &result) == COOP_PROGRESS_UNREADABLE);
    CHECK(!result.mission_number && !result.mission_part);
    CHECK(CoopReadScriptProgress(read_memory, NULL, SLOT, NULL) == COOP_PROGRESS_UNREADABLE);
    puts("PASS production script progress reader; no world identity claimed");
    return 0;
}
