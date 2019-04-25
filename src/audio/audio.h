#include "util.h"

#pragma once

#define MAX_MICROPHONES 8

struct Source;

int lovrAudioConvertFormat(u32 bitDepth, u32 channelCount);

bool lovrAudioInit(void);
void lovrAudioDestroy(void);
void lovrAudioUpdate(void);
void lovrAudioAdd(struct Source* source);
void lovrAudioGetDopplerEffect(f32* factor, f32* speedOfSound);
void lovrAudioGetMicrophoneNames(const char* names[MAX_MICROPHONES], u32* count);
void lovrAudioGetOrientation(f32 orientation[4]);
void lovrAudioGetPosition(f32 position[3]);
void lovrAudioGetVelocity(f32 velocity[3]);
f32 lovrAudioGetVolume(void);
bool lovrAudioHas(struct Source* source);
bool lovrAudioIsSpatialized(void);
void lovrAudioPause(void);
void lovrAudioResume(void);
void lovrAudioRewind(void);
void lovrAudioSetDopplerEffect(f32 factor, f32 speedOfSound);
void lovrAudioSetOrientation(f32 orientation[4]);
void lovrAudioSetPosition(f32 position[3]);
void lovrAudioSetVelocity(f32 velocity[3]);
void lovrAudioSetVolume(f32 volume);
void lovrAudioStop(void);
