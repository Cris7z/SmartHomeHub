#include <cassert>
#include <cstdint>

#include "../src/board/speaker_tone.h"

namespace {
void testSquareToneFlipsAtHalfPeriod() {
  const uint32_t sampleRate = 16000;
  const uint16_t frequency = 1000;
  const int16_t amplitude = 1200;

  assert(speakerSquareSample(0, sampleRate, frequency, amplitude) == amplitude);
  assert(speakerSquareSample(7, sampleRate, frequency, amplitude) == amplitude);
  assert(speakerSquareSample(8, sampleRate, frequency, amplitude) == -amplitude);
  assert(speakerSquareSample(15, sampleRate, frequency, amplitude) == -amplitude);
  assert(speakerSquareSample(16, sampleRate, frequency, amplitude) == amplitude);
}

void testFillStereoToneFramesMirrorsChannels() {
  SpeakerToneFrame frames[4] = {};
  const size_t written = fillSpeakerToneFrames(frames, 4, 0, 16000, 1000, 900);

  assert(written == 4);
  for (const SpeakerToneFrame &frame : frames) {
    assert(frame.left == frame.right);
    assert(frame.left == 900 || frame.left == -900);
  }
}
}

int main() {
  testSquareToneFlipsAtHalfPeriod();
  testFillStereoToneFramesMirrorsChannels();
  return 0;
}
