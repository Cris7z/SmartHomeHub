#include "speaker_tone.h"

int16_t speakerSquareSample(uint32_t sampleIndex, uint32_t sampleRate,
                            uint16_t frequencyHz, int16_t amplitude) {
  if (sampleRate == 0 || frequencyHz == 0 || amplitude == 0) {
    return 0;
  }

  uint32_t periodSamples = sampleRate / frequencyHz;
  if (periodSamples < 2) {
    periodSamples = 2;
  }
  const uint32_t halfPeriod = periodSamples / 2;
  const uint32_t phase = sampleIndex % periodSamples;
  return phase < halfPeriod ? amplitude : (int16_t)-amplitude;
}

size_t fillSpeakerToneFrames(SpeakerToneFrame *frames, size_t frameCount,
                             uint32_t startSampleIndex, uint32_t sampleRate,
                             uint16_t frequencyHz, int16_t amplitude) {
  if (!frames) {
    return 0;
  }

  for (size_t i = 0; i < frameCount; i++) {
    const int16_t sample = speakerSquareSample(
        startSampleIndex + (uint32_t)i, sampleRate, frequencyHz, amplitude);
    frames[i].left = sample;
    frames[i].right = sample;
  }
  return frameCount;
}

size_t fillSpeakerPcmFrames(SpeakerToneFrame *frames, size_t frameCount,
                            const int16_t *samples, size_t sampleCount,
                            size_t startSampleIndex) {
  if (!frames || !samples || startSampleIndex >= sampleCount) {
    return 0;
  }

  const size_t framesAvailable = sampleCount - startSampleIndex;
  const size_t framesToWrite = frameCount < framesAvailable ? frameCount : framesAvailable;
  for (size_t i = 0; i < framesToWrite; i++) {
    const int16_t sample = samples[startSampleIndex + i];
    frames[i].left = sample;
    frames[i].right = sample;
  }
  return framesToWrite;
}
