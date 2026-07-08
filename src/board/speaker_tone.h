#pragma once

#include <cstddef>
#include <cstdint>

struct SpeakerToneFrame {
  int16_t left;
  int16_t right;
};

int16_t speakerSquareSample(uint32_t sampleIndex, uint32_t sampleRate,
                            uint16_t frequencyHz, int16_t amplitude);
size_t fillSpeakerToneFrames(SpeakerToneFrame *frames, size_t frameCount,
                             uint32_t startSampleIndex, uint32_t sampleRate,
                             uint16_t frequencyHz, int16_t amplitude);
size_t fillSpeakerPcmFrames(SpeakerToneFrame *frames, size_t frameCount,
                            const int16_t *samples, size_t sampleCount,
                            size_t startSampleIndex);
