#include "util.h"
#include "data/blob.h"

#pragma once

struct AudioStream;

typedef struct SoundData {
  Blob blob;
  usize samples;
  u32 channelCount;
  u32 sampleRate;
  u32 bitDepth;
} SoundData;

SoundData* lovrSoundDataInit(SoundData* soundData, usize samples, u32 sampleRate, u32 bitDepth, u32 channels);
SoundData* lovrSoundDataInitFromAudioStream(SoundData* soundData, struct AudioStream* audioStream);
SoundData* lovrSoundDataInitFromBlob(SoundData* soundData, Blob* blob);
#define lovrSoundDataCreate(...) lovrSoundDataInit(lovrAlloc(SoundData), __VA_ARGS__)
#define lovrSoundDataCreateFromAudioStream(...) lovrSoundDataInitFromAudioStream(lovrAlloc(SoundData), __VA_ARGS__)
#define lovrSoundDataCreateFromBlob(...) lovrSoundDataInitFromBlob(lovrAlloc(SoundData), __VA_ARGS__)
f32 lovrSoundDataGetSample(SoundData* soundData, usize index);
void lovrSoundDataSetSample(SoundData* soundData, usize index, f32 value);
void lovrSoundDataDestroy(void* ref);
