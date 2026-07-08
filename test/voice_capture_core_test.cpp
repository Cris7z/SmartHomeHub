#include <cassert>
#include <cstdint>

#include "../src/io/voice_capture_core.h"

int main() {
  const int32_t stereo[] = {
      0, 0x00100000, 0, 0x00200000,
      0, static_cast<int32_t>(0xffe00000), 0, static_cast<int32_t>(0xfff00000)};
  assert(selectVoiceI2sSlot(stereo, 8) == 1);

  int16_t pcm[4] = {};
  assert(convertVoiceI2sToPcm16(stereo, 8, 1, 8, pcm, 4) == 4);
  assert(pcm[0] > 0);
  assert(pcm[2] < 0);

  int16_t storage[5] = {};
  VoicePreRoll ring(storage, 5);
  const int16_t first[] = {1, 2, 3};
  const int16_t second[] = {4, 5, 6, 7};
  ring.push(first, 3);
  ring.push(second, 4);
  int16_t copied[5] = {};
  assert(ring.copyChronological(copied, 5) == 5);
  const int16_t expected[] = {3, 4, 5, 6, 7};
  for (int i = 0; i < 5; ++i) {
    assert(copied[i] == expected[i]);
  }
}
