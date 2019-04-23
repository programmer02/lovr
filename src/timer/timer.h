#include "util.h"

#pragma once

bool lovrTimerInit(void);
void lovrTimerDestroy(void);
f64 lovrTimerGetDelta(void);
f64 lovrTimerGetTime(void);
f64 lovrTimerStep(void);
f64 lovrTimerGetAverageDelta(void);
u32 lovrTimerGetFPS(void);
void lovrTimerSleep(f64 seconds);
