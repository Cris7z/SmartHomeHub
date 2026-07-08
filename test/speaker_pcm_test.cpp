#include <cassert>
#include <cstdint>

#include "../src/board/speaker_tone.h"

namespace {
void testPcmClipCopiesMonoSamplesToStereoFrames() {
  const int16_t samples[] = {-120, 0, 240};
  SpeakerToneFrame frames[4] = {};

  const size_t written = fillSpeakerPcmFrames(frames, 4, samples, 3, 1);

  assert(written == 2);
  assert(frames[0].left == 0);
  assert(frames[0].right == 0);
  assert(frames[1].left == 240);
  assert(frames[1].right == 240);
  assert(frames[2].left == 0);
  assert(frames[2].right == 0);
}

void testPcmClipRejectsInvalidInputs() {
  SpeakerToneFrame frames[2] = {};
  const int16_t samples[] = {42};

  assert(fillSpeakerPcmFrames(nullptr, 2, samples, 1, 0) == 0);
  assert(fillSpeakerPcmFrames(frames, 2, nullptr, 1, 0) == 0);
  assert(fillSpeakerPcmFrames(frames, 2, samples, 1, 1) == 0);
}
}

int main() {
  testPcmClipCopiesMonoSamplesToStereoFrames();
  testPcmClipRejectsInvalidInputs();
  return 0;
}
