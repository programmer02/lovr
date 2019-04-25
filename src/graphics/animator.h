#include "util.h"
#include "types.h"
#include "lib/maf.h"
#include "lib/map/map.h"
#include "lib/vec/vec.h"

#pragma once

struct ModelData;

typedef struct {
  f32 time;
  f32 speed;
  f32 alpha;
  i32 priority;
  bool playing;
  bool looping;
} Track;

typedef vec_t(Track) vec_track_t;

typedef struct Animator {
  Ref ref;
  struct ModelData* data;
  map_t(u32) animations;
  vec_track_t tracks;
  f32 speed;
} Animator;

Animator* lovrAnimatorInit(Animator* animator, struct ModelData* modelData);
#define lovrAnimatorCreate(...) lovrAnimatorInit(lovrAlloc(Animator), __VA_ARGS__)
void lovrAnimatorDestroy(void* ref);
void lovrAnimatorReset(Animator* animator);
void lovrAnimatorUpdate(Animator* animator, f32 dt);
bool lovrAnimatorEvaluate(Animator* animator, u32 nodeIndex, mat4 transform);
u32 lovrAnimatorGetAnimationCount(Animator* animator);
u32* lovrAnimatorGetAnimationIndex(Animator* animator, const char* name);
const char* lovrAnimatorGetAnimationName(Animator* animator, u32 index);
void lovrAnimatorPlay(Animator* animator, u32 animation);
void lovrAnimatorStop(Animator* animator, u32 animation);
void lovrAnimatorPause(Animator* animator, u32 animation);
void lovrAnimatorResume(Animator* animator, u32 animation);
void lovrAnimatorSeek(Animator* animator, u32 animation, f32 time);
f32 lovrAnimatorTell(Animator* animator, u32 animation);
f32 lovrAnimatorGetAlpha(Animator* animator, u32 animation);
void lovrAnimatorSetAlpha(Animator* animator, u32 animation, f32 alpha);
f32 lovrAnimatorGetDuration(Animator* animator, u32 animation);
bool lovrAnimatorIsPlaying(Animator* animator, u32 animation);
bool lovrAnimatorIsLooping(Animator* animator, u32 animation);
void lovrAnimatorSetLooping(Animator* animator, u32 animation, bool loop);
i32 lovrAnimatorGetPriority(Animator* animator, u32 animation);
void lovrAnimatorSetPriority(Animator* animator, u32 animation, i32 priority);
f32 lovrAnimatorGetSpeed(Animator* animator, u32 animation);
void lovrAnimatorSetSpeed(Animator* animator, u32 animation, f32 speed);
