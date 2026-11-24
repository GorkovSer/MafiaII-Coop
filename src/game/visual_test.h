#ifndef COOP_VISUAL_TEST_H
#define COOP_VISUAL_TEST_H
#include <wchar.h>
/* Explicit offline isolated-profile acceptance only. No network authority. */
int CoopVisualTestStart(const wchar_t *path);
void CoopVisualTestEvent(unsigned int event);
#endif
