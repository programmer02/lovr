#include "data/audioStream.h"
#include "data/blob.h"
#include "lib/err.h"
#include "lib/stb/stb_vorbis.h"
#include <stdlib.h>

AudioStream* lovrAudioStreamInit(AudioStream* stream, Blob* blob, usize bufferSize) {
  stb_vorbis* decoder = stb_vorbis_open_memory(blob->data, (i32) blob->size, NULL, NULL);
  lovrAssert(decoder, "Could not create audio stream for '%s'", blob->name);

  stb_vorbis_info info = stb_vorbis_get_info(decoder);

  stream->bitDepth = 16;
  stream->channelCount = info.channels;
  stream->sampleRate = info.sample_rate;
  stream->samples = stb_vorbis_stream_length_in_samples(decoder);
  stream->decoder = decoder;
  stream->bufferSize = stream->channelCount * bufferSize * sizeof(i16);
  stream->buffer = malloc(stream->bufferSize);
  lovrAssert(stream->buffer, "Out of memory");
  stream->blob = blob;
  lovrRetain(blob);

  return stream;
}

void lovrAudioStreamDestroy(void* ref) {
  AudioStream* stream = ref;
  stb_vorbis_close(stream->decoder);
  lovrRelease(Blob, stream->blob);
  free(stream->buffer);
}

usize lovrAudioStreamDecode(AudioStream* stream, i16* destination, usize size) {
  stb_vorbis* decoder = (stb_vorbis*) stream->decoder;
  i16* buffer = destination ? destination : (i16*) stream->buffer;
  usize capacity = destination ? size : (stream->bufferSize / sizeof(i16));
  u32 channelCount = stream->channelCount;
  usize samples = 0;

  while (samples < capacity) {
    usize count = (usize) stb_vorbis_get_samples_short_interleaved(decoder, channelCount, buffer + samples, capacity - samples);
    if (count == 0) break;
    samples += count * channelCount;
  }

  return samples;
}

void lovrAudioStreamRewind(AudioStream* stream) {
  stb_vorbis* decoder = (stb_vorbis*) stream->decoder;
  stb_vorbis_seek_start(decoder);
}

void lovrAudioStreamSeek(AudioStream* stream, usize sample) {
  stb_vorbis* decoder = (stb_vorbis*) stream->decoder;
  stb_vorbis_seek(decoder, sample);
}

usize lovrAudioStreamTell(AudioStream* stream) {
  stb_vorbis* decoder = (stb_vorbis*) stream->decoder;
  return (usize) stb_vorbis_get_sample_offset(decoder);
}
