#include "math/randomGenerator.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

// Thomas Wang's 64-bit integer hashing function:
// https://web.archive.org/web/20110807030012/http://www.cris.com/%7ETtwang/tech/inthash.htm
static u64 wangHash64(u64 key) {
  key = (~key) + (key << 21); // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8); // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4); // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}

// 64 bit Xorshift implementation taken from the end of Sec. 3 (page 4) in
// George Marsaglia, "Xorshift RNGs", Journal of Statistical Software, Vol.8 (Issue 14), 2003
// Use an 'Xorshift*' variant, as shown here: http://xorshift.di.unimi.it

RandomGenerator* lovrRandomGeneratorInit(RandomGenerator* generator) {
  Seed seed = { .b32 = { .lo = 0xCBBF7A44, .hi = 0x0139408D } };
  lovrRandomGeneratorSetSeed(generator, seed);
  generator->lastRandomNormal = HUGE_VAL;
  return generator;
}

void lovrRandomGeneratorDestroy(void* ref) {
  //
}

Seed lovrRandomGeneratorGetSeed(RandomGenerator* generator) {
  return generator->seed;
}

void lovrRandomGeneratorSetSeed(RandomGenerator* generator, Seed seed) {
  generator->seed = seed;

  do {
    seed.b64 = wangHash64(seed.b64);
  } while (seed.b64 == 0);

  generator->state = seed;
}

void lovrRandomGeneratorGetState(RandomGenerator* generator, char* state, usize length) {
  snprintf(state, length, "0x%" PRIx64, generator->state.b64);
}

bool lovrRandomGeneratorSetState(RandomGenerator* generator, const char* state) {
  char* end = NULL;
  Seed newState;
  newState.b64 = strtoull(state, &end, 16);
  if (end != NULL && *end != '\0') {
    return false;
  } else {
    generator->state = newState;
    return true;
  }
}

f64 lovrRandomGeneratorRandom(RandomGenerator* generator) {
  generator->state.b64 ^= (generator->state.b64 >> 12);
  generator->state.b64 ^= (generator->state.b64 << 25);
  generator->state.b64 ^= (generator->state.b64 >> 27);
  u64 r = generator->state.b64 * 2685821657736338717ULL;
  union { u64 i; f64 d; } u;
  u.i = ((0x3FFULL) << 52) | (r >> 12);
  return u.d - 1.;
}

f64 lovrRandomGeneratorRandomNormal(RandomGenerator* generator) {
  if (generator->lastRandomNormal != HUGE_VAL) {
    f64 r = generator->lastRandomNormal;
    generator->lastRandomNormal = HUGE_VAL;
    return r;
  }

  f64 a = lovrRandomGeneratorRandom(generator);
  f64 b = lovrRandomGeneratorRandom(generator);
  f64 r = sqrt(-2. * log(1. - a));
  f64 phi = 2. * M_PI * (1. - b);
  generator->lastRandomNormal = r * cos(phi);
  return r * sin(phi);
}
