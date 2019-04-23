#include "util.h"
#include "types.h"

#pragma once

// Direct port of LÖVE's RandomGenerator

typedef union {
  u64 b64;
  struct {
    u32 lo;
    u32 hi;
  } b32;
} Seed;

typedef struct RandomGenerator {
  Ref ref;
  Seed seed;
  Seed state;
  f64 lastRandomNormal;
} RandomGenerator;

RandomGenerator* lovrRandomGeneratorInit(RandomGenerator* generator);
#define lovrRandomGeneratorCreate() lovrRandomGeneratorInit(lovrAlloc(RandomGenerator))
void lovrRandomGeneratorDestroy(void* ref);
Seed lovrRandomGeneratorGetSeed(RandomGenerator* generator);
void lovrRandomGeneratorSetSeed(RandomGenerator* generator, Seed seed);
void lovrRandomGeneratorGetState(RandomGenerator* generator, char* state, usize length);
bool lovrRandomGeneratorSetState(RandomGenerator* generator, const char* state);
f64 lovrRandomGeneratorRandom(RandomGenerator* generator);
f64 lovrRandomGeneratorRandomNormal(RandomGenerator* generator);
