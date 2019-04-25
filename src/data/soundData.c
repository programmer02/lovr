#include "data/soundData.h"
#include "data/audioStream.h"
#include "lib/err.h"
#include "lib/stb/stb_vorbis.h"
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

SoundData* lovrSoundDataInit(SoundData* soundData, usize samples, u32 sampleRate, u32 bitDepth, u32 channelCount) {
  soundData->samples = samples;
  soundData->sampleRate = sampleRate;
  soundData->bitDepth = bitDepth;
  soundData->channelCount = channelCount;
  soundData->blob.size = samples * channelCount * (bitDepth / 8);
  soundData->blob.data = calloc(1, soundData->blob.size);
  lovrAssert(soundData->blob.data, "Out of memory");
  return soundData;
}

SoundData* lovrSoundDataInitFromAudioStream(SoundData* soundData, AudioStream* audioStream) {
  soundData->samples = audioStream->samples;
  soundData->sampleRate = audioStream->sampleRate;
  soundData->bitDepth = audioStream->bitDepth;
  soundData->channelCount = audioStream->channelCount;
  soundData->blob.size = audioStream->samples * audioStream->channelCount * (audioStream->bitDepth / 8);
  soundData->blob.data = calloc(1, soundData->blob.size);
  lovrAssert(soundData->blob.data, "Out of memory");

  usize samples;
  i16* buffer = soundData->blob.data;
  usize offset = 0;
  lovrAudioStreamRewind(audioStream);
  while ((samples = lovrAudioStreamDecode(audioStream, buffer + offset, soundData->blob.size - (offset * sizeof(i16)))) != 0) {
    offset += samples;
  }

  return soundData;
}

SoundData* lovrSoundDataInitFromBlob(SoundData* soundData, Blob* blob) {
  soundData->bitDepth = 16;
  int channels = soundData->channelCount;
  int sampleRate = soundData->sampleRate;
  soundData->samples = stb_vorbis_decode_memory(blob->data, (int) blob->size, &channels, &sampleRate, (short**) &soundData->blob.data);
  soundData->blob.size = soundData->samples * soundData->channelCount * (soundData->bitDepth / 8);
  return soundData;
}

f32 lovrSoundDataGetSample(SoundData* soundData, usize index) {
  lovrAssert(index < soundData->blob.size / (soundData->bitDepth / 8), "Sample index out of range");
  switch (soundData->bitDepth) {
    case 8: return ((i8*) soundData->blob.data)[index] / (f32) CHAR_MAX;
    case 16: return ((i16*) soundData->blob.data)[index] / (f32) SHRT_MAX;
    default: lovrThrow("Unsupported SoundData bit depth %u\n", soundData->bitDepth); return 0;
  }
}

void lovrSoundDataSetSample(SoundData* soundData, usize index, f32 value) {
  lovrAssert(index < soundData->blob.size / (soundData->bitDepth / 8), "Sample index out of range");
  switch (soundData->bitDepth) {
    case 8: ((i8*) soundData->blob.data)[index] = value * CHAR_MAX; break;
    case 16: ((i16*) soundData->blob.data)[index] = value * SHRT_MAX; break;
    default: lovrThrow("Unsupported SoundData bit depth %u\n", soundData->bitDepth); break;
  }
}

void lovrSoundDataDestroy(void* ref) {
  lovrBlobDestroy(ref);
}
