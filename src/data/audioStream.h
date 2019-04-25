#include "util.h"
#include "types.h"

#pragma once

struct Blob;

typedef struct AudioStream {
  Ref ref;
  u32 bitDepth;
  u32 channelCount;
  u32 sampleRate;
  usize samples;
  u32 bufferSize;
  void* buffer;
  void* decoder;
  struct Blob* blob;
} AudioStream;

AudioStream* lovrAudioStreamInit(AudioStream* stream, struct Blob* blob, usize bufferSize);
#define lovrAudioStreamCreate(...) lovrAudioStreamInit(lovrAlloc(AudioStream), __VA_ARGS__)
void lovrAudioStreamDestroy(void* ref);
usize lovrAudioStreamDecode(AudioStream* stream, i16* destination, usize size);
void lovrAudioStreamRewind(AudioStream* stream);
void lovrAudioStreamSeek(AudioStream* stream, usize sample);
usize lovrAudioStreamTell(AudioStream* stream);
