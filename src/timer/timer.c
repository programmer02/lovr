#include "timer.h"
#include "platform.h"
#include <string.h>

#define TICK_SAMPLES 90

static struct {
  bool initialized;
  f64 lastTime;
  f64 time;
  f64 dt;
  f64 tickSum;
  f64 tickBuffer[TICK_SAMPLES];
  f64 averageDelta;
  usize tickIndex;
  u32 fps;
} state;

bool lovrTimerInit() {
  if (state.initialized) return false;
  lovrTimerDestroy();
  return state.initialized = true;
}

void lovrTimerDestroy() {
  if (!state.initialized) return;
  memset(&state, 0, sizeof(state));
}

f64 lovrTimerGetDelta() {
  return state.dt;
}

f64 lovrTimerGetTime() {
  return lovrPlatformGetTime();
}

f64 lovrTimerStep() {
  state.lastTime = state.time;
  state.time = lovrPlatformGetTime();
  state.dt = state.time - state.lastTime;
  state.tickSum -= state.tickBuffer[state.tickIndex];
  state.tickSum += state.dt;
  state.tickBuffer[state.tickIndex] = state.dt;
  state.averageDelta = state.tickSum / TICK_SAMPLES;
  state.fps = (u32) (1. / (state.tickSum / TICK_SAMPLES) + .5);
  if (++state.tickIndex == TICK_SAMPLES) {
    state.tickIndex = 0u;
  }
  return state.dt;
}

f64 lovrTimerGetAverageDelta() {
  return state.averageDelta;
}

u32 lovrTimerGetFPS() {
  return state.fps;
}

void lovrTimerSleep(f64 seconds) {
  lovrSleep(seconds);
}
