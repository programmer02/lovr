#include "util.h"

#pragma once

struct RandomGenerator;

bool lovrMathInit(void);
void lovrMathDestroy(void);
struct RandomGenerator* lovrMathGetRandomGenerator(void);
void lovrMathOrientationToDirection(f32 angle, f32 ax, f32 ay, f32 az, f32 v[3]);
f32 lovrMathGammaToLinear(f32 x);
f32 lovrMathLinearToGamma(f32 x);
f32 lovrMathNoise1(f32 x);
f32 lovrMathNoise2(f32 x, f32 y);
f32 lovrMathNoise3(f32 x, f32 y, f32 z);
f32 lovrMathNoise4(f32 x, f32 y, f32 z, f32 w);
