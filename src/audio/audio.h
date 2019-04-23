#include "util.h"

#pragma once

#define MAX_MICROPHONES 8

struct Source;

int lovrAudioConvertFormat(int bitDepth, int channelCount);

bool lovrAudioInit(void);
void lovrAudioDestroy(void);
void lovrAudioUpdate(void);
void lovrAudioAdd(struct Source* source);
void lovrAudioGetDopplerEffect(f32* factor, f32* speedOfSound);
void lovrAudioGetMicrophoneNames(const char* names[MAX_MICROPHONES], u8* count);
void lovrAudioGetOrientation(f32* orientation);
void lovrAudioGetPosition(f32* position);
void lovrAudioGetVelocity(f32* velocity);
f32 lovrAudioGetVolume(void);
bool lovrAudioHas(struct Source* source);
bool lovrAudioIsSpatialized(void);
void lovrAudioPause(void);
void lovrAudioResume(void);
void lovrAudioRewind(void);
void lovrAudioSetDopplerEffect(f32 factor, f32 speedOfSound);
void lovrAudioSetOrientation(f32* orientation);
void lovrAudioSetPosition(f32* position);
void lovrAudioSetVelocity(f32* velocity);
void lovrAudioSetVolume(f32 volume);
void lovrAudioStop(void);
