#include <cassert>
#include <cstdint>

#include "../src/board/xiaozhi_voice_clip.h"

int main() {
  assert(XIAOZHI_HELLO_SAMPLE_RATE == 16000);
  assert(XIAOZHI_HELLO_PCM_SAMPLE_COUNT > 8000);
  assert(XIAOZHI_HELLO_PCM_SAMPLE_COUNT <= 27000);

  int32_t peak = 0;
  for (size_t i = 0; i < XIAOZHI_HELLO_PCM_SAMPLE_COUNT; i++) {
    const int32_t sample = XIAOZHI_HELLO_PCM[i] < 0
        ? -(int32_t)XIAOZHI_HELLO_PCM[i]
        : (int32_t)XIAOZHI_HELLO_PCM[i];
    if (sample > peak) {
      peak = sample;
    }
  }
  assert(peak > 1000);
  assert(peak < 30000);
  return 0;
}
