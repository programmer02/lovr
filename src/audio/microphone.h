#include "util.h"
#include "types.h"
#include <AL/al.h>
#include <AL/alc.h>

#pragma once

struct SoundData;

typedef struct Microphone {
  Ref ref;
  ALCdevice* device;
  const char* name;
  bool isRecording;
  u32 sampleRate;
  u32 bitDepth;
  u32 channelCount;
} Microphone;

Microphone* lovrMicrophoneInit(Microphone* microphone, const char* name, usize samples, u32 sampleRate, u32 bitDepth, u32 channelCount);
#define lovrMicrophoneCreate(...) lovrMicrophoneInit(lovrAlloc(Microphone), __VA_ARGS__)
void lovrMicrophoneDestroy(void* ref);
u32 lovrMicrophoneGetBitDepth(Microphone* microphone);
u32 lovrMicrophoneGetChannelCount(Microphone* microphone);
struct SoundData* lovrMicrophoneGetData(Microphone* microphone);
const char* lovrMicrophoneGetName(Microphone* microphone);
usize lovrMicrophoneGetSampleCount(Microphone* microphone);
u32 lovrMicrophoneGetSampleRate(Microphone* microphone);
bool lovrMicrophoneIsRecording(Microphone* microphone);
void lovrMicrophoneStartRecording(Microphone* microphone);
void lovrMicrophoneStopRecording(Microphone* microphone);
