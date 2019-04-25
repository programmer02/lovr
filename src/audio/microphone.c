#include "audio/microphone.h"
#include "audio/audio.h"
#include "data/soundData.h"
#include "types.h"
#include <AL/al.h>
#include <AL/alc.h>

struct Microphone {
  Ref ref;
  ALCdevice* device;
  const char* name;
  bool isRecording;
  u32 sampleRate;
  u32 bitDepth;
  u32 channelCount;
};

const usize sizeof_Microphone = sizeof(Microphone);

Microphone* lovrMicrophoneInit(Microphone* microphone, const char* name, usize samples, u32 sampleRate, u32 bitDepth, u32 channelCount) {
  ALCdevice* device = alcCaptureOpenDevice(name, sampleRate, lovrAudioConvertFormat(bitDepth, channelCount), samples);
  lovrAssert(device, "Error opening capture device for microphone '%s'", name);
  microphone->device = device;
  microphone->name = name ? name : alcGetString(device, ALC_CAPTURE_DEVICE_SPECIFIER);
  microphone->sampleRate = sampleRate;
  microphone->bitDepth = bitDepth;
  microphone->channelCount = channelCount;
  return microphone;
}

void lovrMicrophoneDestroy(void* ref) {
  Microphone* microphone = ref;
  lovrMicrophoneStopRecording(microphone);
  alcCaptureCloseDevice(microphone->device);
}

u32 lovrMicrophoneGetBitDepth(Microphone* microphone) {
  return microphone->bitDepth;
}

u32 lovrMicrophoneGetChannelCount(Microphone* microphone) {
  return microphone->channelCount;
}

SoundData* lovrMicrophoneGetData(Microphone* microphone) {
  if (!microphone->isRecording) {
    return NULL;
  }

  usize samples = lovrMicrophoneGetSampleCount(microphone);
  if (samples == 0) {
    return NULL;
  }

  SoundData* soundData = lovrSoundDataCreate(samples, microphone->sampleRate, microphone->bitDepth, microphone->channelCount);
  alcCaptureSamples(microphone->device, soundData->blob.data, samples);
  return soundData;
}

const char* lovrMicrophoneGetName(Microphone* microphone) {
  return microphone->name;
}

usize lovrMicrophoneGetSampleCount(Microphone* microphone) {
  if (!microphone->isRecording) {
    return 0;
  }

  ALCint samples;
  alcGetIntegerv(microphone->device, ALC_CAPTURE_SAMPLES, sizeof(ALCint), &samples);
  return (usize) samples;
}

u32 lovrMicrophoneGetSampleRate(Microphone* microphone) {
  return microphone->sampleRate;
}

bool lovrMicrophoneIsRecording(Microphone* microphone) {
  return microphone->isRecording;
}

void lovrMicrophoneStartRecording(Microphone* microphone) {
  if (microphone->isRecording) {
    return;
  }

  alcCaptureStart(microphone->device);
  microphone->isRecording = true;
}

void lovrMicrophoneStopRecording(Microphone* microphone) {
  if (!microphone->isRecording) {
    return;
  }

  alcCaptureStop(microphone->device);
  microphone->isRecording = false;
}
