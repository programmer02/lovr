#include "util.h"
#include "types.h"
#include <AL/al.h>
#include <AL/alc.h>

#pragma once

#define SOURCE_BUFFERS 4

struct AudioStream;
struct SoundData;

typedef enum {
  SOURCE_STATIC,
  SOURCE_STREAM
} SourceType;

typedef enum {
  UNIT_SECONDS,
  UNIT_SAMPLES
} TimeUnit;

typedef struct Source {
  Ref ref;
  SourceType type;
  struct SoundData* soundData;
  struct AudioStream* stream;
  ALuint id;
  ALuint buffers[SOURCE_BUFFERS];
  bool isLooping;
} Source;

Source* lovrSourceInitStatic(Source* source, struct SoundData* soundData);
Source* lovrSourceInitStream(Source* source, struct AudioStream* stream);
#define lovrSourceCreateStatic(...) lovrSourceInitStatic(lovrAlloc(Source), __VA_ARGS__)
#define lovrSourceCreateStream(...) lovrSourceInitStream(lovrAlloc(Source), __VA_ARGS__)
void lovrSourceDestroy(void* ref);
SourceType lovrSourceGetType(Source* source);
u32 lovrSourceGetBitDepth(Source* source);
u32 lovrSourceGetChannelCount(Source* source);
void lovrSourceGetCone(Source* source, f32* innerAngle, f32* outerAngle, f32* outerGain);
void lovrSourceGetOrientation(Source* source, f32 orientation[4]);
usize lovrSourceGetDuration(Source* source);
void lovrSourceGetFalloff(Source* source, f32* reference, f32* max, f32* rolloff);
f32 lovrSourceGetPitch(Source* source);
void lovrSourceGetPosition(Source* source, f32 position[3]);
void lovrSourceGetVelocity(Source* source, f32 velocity[3]);
u32 lovrSourceGetSampleRate(Source* source);
f32 lovrSourceGetVolume(Source* source);
void lovrSourceGetVolumeLimits(Source* source, f32* min, f32* max);
bool lovrSourceIsLooping(Source* source);
bool lovrSourceIsPaused(Source* source);
bool lovrSourceIsPlaying(Source* source);
bool lovrSourceIsRelative(Source* source);
bool lovrSourceIsStopped(Source* source);
void lovrSourcePause(Source* source);
void lovrSourcePlay(Source* source);
void lovrSourceResume(Source* source);
void lovrSourceRewind(Source* source);
void lovrSourceSeek(Source* source, usize sample);
void lovrSourceSetCone(Source* source, f32 inner, f32 outer, f32 outerGain);
void lovrSourceSetOrientation(Source* source, f32 orientation[4]);
void lovrSourceSetFalloff(Source* source, f32 reference, f32 max, f32 rolloff);
void lovrSourceSetLooping(Source* source, bool isLooping);
void lovrSourceSetPitch(Source* source, f32 pitch);
void lovrSourceSetPosition(Source* source, f32 position[3]);
void lovrSourceSetRelative(Source* source, bool isRelative);
void lovrSourceSetVelocity(Source* source, f32 velocity[3]);
void lovrSourceSetVolume(Source* source, f32 volume);
void lovrSourceSetVolumeLimits(Source* source, f32 min, f32 max);
void lovrSourceStop(Source* source);
void lovrSourceStream(Source* source, ALuint* buffers, usize count);
usize lovrSourceTell(Source* source);
