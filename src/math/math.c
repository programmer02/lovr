#include "math.h"
#include "math/randomGenerator.h"
#include "lib/maf.h"
#include "lib/noise1234/noise1234.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static struct {
  bool initialized;
  RandomGenerator* generator;
} state;

bool lovrMathInit() {
  if (state.initialized) return false;
  state.generator = lovrRandomGeneratorCreate();
  Seed seed = { .b64 = (u64) time(0) };
  lovrRandomGeneratorSetSeed(state.generator, seed);
  return state.initialized = true;
}

void lovrMathDestroy() {
  if (!state.initialized) return;
  lovrRelease(RandomGenerator, state.generator);
  memset(&state, 0, sizeof(state));
}

RandomGenerator* lovrMathGetRandomGenerator() {
  return state.generator;
}

void lovrMathOrientationToDirection(f32 angle, f32 ax, f32 ay, f32 az, vec3 v) {
  f32 sinTheta = sinf(angle);
  f32 cosTheta = cosf(angle);
  v[0] = sinTheta * -ay + (1.f - cosTheta) * -az * ax;
  v[1] = sinTheta * ax + (1.f - cosTheta) * -az * ay;
  v[2] = -cosTheta + (1.f - cosTheta) * -az * az;
}

f32 lovrMathGammaToLinear(f32 x) {
  if (x <= .04045f) {
    return x / 12.92f;
  } else {
    return powf((x + .055f) / 1.055f, 2.4f);
  }
}

f32 lovrMathLinearToGamma(f32 x) {
  if (x <= .0031308f) {
    return x * 12.92f;
  } else {
    return 1.055f * powf(x, 1.f / 2.4f) - .055f;
  }
}

f32 lovrMathNoise1(f32 x) {
  return noise1(x) * .5f + .5f;
}

f32 lovrMathNoise2(f32 x, f32 y) {
  return noise2(x, y) * .5f + .5f;
}

f32 lovrMathNoise3(f32 x, f32 y, f32 z) {
  return noise3(x, y, z) * .5f + .5f;
}

f32 lovrMathNoise4(f32 x, f32 y, f32 z, f32 w) {
  return noise4(x, y, z, w) * .5f + .5f;
}
