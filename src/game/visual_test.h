#ifndef COOP_VISUAL_TEST_H
#define COOP_VISUAL_TEST_H
#include <wchar.h>
/* Opt-in visual-only renderer or isolated offline batch. No Human/AI/campaign authority. */
int CoopVisualTestStart(const wchar_t *path);
void CoopVisualTestEvent(unsigned int event);
#endif
