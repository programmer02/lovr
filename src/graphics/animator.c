#include "graphics/animator.h"
#include "data/modelData.h"
#include "types.h"
#include "lib/err.h"
#include "lib/maf.h"
#include "lib/map/map.h"
#include "lib/vec/vec.h"
#include <stdlib.h>
#include <math.h>

typedef struct {
  f32 time;
  f32 speed;
  f32 alpha;
  i32 priority;
  bool playing;
  bool looping;
} Track;

struct Animator {
  Ref ref;
  struct ModelData* data;
  map_t(u32) animations;
  vec_t(Track) tracks;
  f32 speed;
};

const usize sizeof_Animator = sizeof(Animator);

static int trackSortCallback(const void* a, const void* b) {
  return ((Track*) a)->priority < ((Track*) b)->priority;
}

Animator* lovrAnimatorInit(Animator* animator, ModelData* data) {
  lovrRetain(data);
  animator->data = data;
  map_init(&animator->animations);
  vec_init(&animator->tracks);
  vec_reserve(&animator->tracks, data->animationCount);
  animator->speed = 1.f;

  for (u32 i = 0; i < data->animationCount; i++) {
    vec_push(&animator->tracks, ((Track) {
      .time = 0.f,
      .speed = 1.f,
      .alpha = 1.f,
      .priority = 0,
      .playing = false,
      .looping = false
    }));

    if (data->animations[i].name) {
      map_set(&animator->animations, data->animations[i].name, i);
    }
  }

  return animator;
}

void lovrAnimatorDestroy(void* ref) {
  Animator* animator = ref;
  lovrRelease(ModelData, animator->data);
  vec_deinit(&animator->tracks);
}

void lovrAnimatorReset(Animator* animator) {
  Track* track; i32 i;
  vec_foreach_ptr(&animator->tracks, track, i) {
    track->time = 0.f;
    track->speed = 1.f;
    track->playing = false;
    track->looping = false;
  }
  animator->speed = 1.f;
}

void lovrAnimatorUpdate(Animator* animator, f32 dt) {
  Track* track; i32 i;
  vec_foreach_ptr(&animator->tracks, track, i) {
    if (track->playing) {
      track->time += dt * track->speed * animator->speed;
      f32 duration = animator->data->animations[i].duration;

      if (track->looping) {
        track->time = fmodf(track->time, duration);
      } else if (track->time > duration || track->time < 0.f) {
        track->time = 0.f;
        track->playing = false;
      }
    }
  }
}

bool lovrAnimatorEvaluate(Animator* animator, u32 nodeIndex, mat4 transform) {
  f32 properties[3][4] = { { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f } };
  bool touched = false;

  for (u32 i = 0; i < animator->data->animationCount; i++) {
    ModelAnimation* animation = &animator->data->animations[i];

    for (u32 j = 0; j < animation->channelCount; j++) {
      ModelAnimationChannel* channel = &animation->channels[j];
      if (channel->nodeIndex != nodeIndex) {
        continue;
      }

      Track* track = &animator->tracks.data[i];
      if (!track->playing || track->alpha == 0.f) {
        continue;
      }

      f32 duration = animator->data->animations[i].duration;
      f32 time = fmodf(track->time, duration);
      u32 k = 0;

      while (k < channel->keyframeCount && channel->times[k] < time) {
        k++;
      }

      f32 value[4];
      bool rotate = channel->property == PROP_ROTATION;
      usize n = 3 + rotate;
      f32* (*lerp)(f32* a, f32* b, f32 t) = rotate ? quat_slerp : vec3_lerp;

      if (k > 0 && k < channel->keyframeCount) {
        f32 t1 = channel->times[k - 1];
        f32 t2 = channel->times[k];
        f32 z = (time - t1) / (t2 - t1);
        f32 next[4];

        memcpy(value, channel->data + (k - 1) * n, n * sizeof(f32));
        memcpy(next, channel->data + k * n, n * sizeof(f32));

        switch (channel->smoothing) {
          case SMOOTH_STEP:
            if (z >= .5f) {
              memcpy(value, next, n * sizeof(f32));
            }
            break;
          case SMOOTH_LINEAR: lerp(value, next, z); break;
          case SMOOTH_CUBIC: lovrThrow("Cubic spline interpolation is not supported yet"); break;
          default: break;
        }
      } else {
        memcpy(value, channel->data + CLAMP(k, 0, channel->keyframeCount - 1) * n, n * sizeof(f32));
      }

      if (track->alpha == 1.f) {
        memcpy(properties[channel->property], value, n * sizeof(f32));
      } else {
        lerp(properties[channel->property], value, track->alpha);
      }

      touched = true;
    }
  }

  if (touched) {
    vec3 T = properties[PROP_TRANSLATION];
    quat R = properties[PROP_ROTATION];
    vec3 S = properties[PROP_SCALE];
    mat4_translate(transform, T[0], T[1], T[2]);
    mat4_rotateQuat(transform, R);
    mat4_scale(transform, S[0], S[1], S[2]);
  }

  return touched;
}

u32 lovrAnimatorGetAnimationCount(Animator* animator) {
  return animator->data->animationCount;
}

u32* lovrAnimatorGetAnimationIndex(Animator* animator, const char* name) {
  return map_get(&animator->animations, name);
}

const char* lovrAnimatorGetAnimationName(Animator* animator, u32 index) {
  return animator->data->animations[index].name;
}

void lovrAnimatorPlay(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  track->playing = true;
  track->time = 0.f;
}

void lovrAnimatorStop(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  track->playing = false;
  track->time = 0.f;
}

void lovrAnimatorPause(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  track->playing = false;
}

void lovrAnimatorResume(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  track->playing = true;
}

void lovrAnimatorSeek(Animator* animator, u32 animation, f32 time) {
  Track* track = &animator->tracks.data[animation];
  f32 duration = animator->data->animations[animation].duration;

  while (time > duration) {
    time -= duration;
  }

  while (time < 0.f) {
    time += duration;
  }

  track->time = time;

  if (!track->looping) {
    track->time = MIN(track->time, duration);
    track->time = MAX(track->time, 0.f);
  }
}

f32 lovrAnimatorTell(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  return track->time;
}

f32 lovrAnimatorGetAlpha(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  return track->alpha;
}

void lovrAnimatorSetAlpha(Animator* animator, u32 animation, f32 alpha) {
  Track* track = &animator->tracks.data[animation];
  track->alpha = alpha;
}

f32 lovrAnimatorGetDuration(Animator* animator, u32 animation) {
  return animator->data->animations[animation].duration;
}

bool lovrAnimatorIsPlaying(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  return track->playing;
}

bool lovrAnimatorIsLooping(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  return track->looping;
}

void lovrAnimatorSetLooping(Animator* animator, u32 animation, bool loop) {
  Track* track = &animator->tracks.data[animation];
  track->looping = loop;
}

int lovrAnimatorGetPriority(Animator* animator, u32 animation) {
  Track* track = &animator->tracks.data[animation];
  return track->priority;
}

void lovrAnimatorSetPriority(Animator* animator, u32 animation, i32 priority) {
  Track* track = &animator->tracks.data[animation];
  track->priority = priority;
  vec_sort(&animator->tracks, trackSortCallback);
}

f32 lovrAnimatorGetSpeed(Animator* animator, u32 animation) {
  if (animation == ~0u) {
    return animator->speed;
  }

  Track* track = &animator->tracks.data[animation];
  return track->speed;
}

void lovrAnimatorSetSpeed(Animator* animator, u32 animation, f32 speed) {
  if (animation == ~0u) {
    animator->speed = speed;
  }

  Track* track = &animator->tracks.data[animation];
  track->speed = speed;
}
