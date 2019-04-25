#include "util.h"

#pragma once

struct SoundData;

typedef struct Microphone Microphone;
extern const usize sizeof_Microphone;
Microphone* lovrMicrophoneInit(Microphone* microphone, const char* name, usize samples, u32 sampleRate, u32 bitDepth, u32 channelCount);
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
