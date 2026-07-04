#include <cassert>
#include <cstdint>

#include "../src/io/mic_processing.h"

int main() {
  const int shift = 8;

  int32_t dcOnly[] = {
    1000000, 1000000, 1000000, 1000000,
    1000000, 1000000, 1000000, 1000000
  };
  MicAnalysis dc = analyzeI2sMicSamples(dcOnly, 8, shift);
  assert(dc.valid);
  assert(dc.sampleCount == 4);
  assert(dc.rms == 0);
  assert(dc.acPeak == 0);

  int32_t stereoSlots[] = {
    0, 1000000,
    0, 1600000,
    0, 1000000,
    0, 400000
  };
  MicAnalysis selected = analyzeI2sMicSamples(stereoSlots, 8, shift);
  assert(selected.valid);
  assert(selected.sampleCount == 4);
  assert(selected.rms > 1000);
  assert(selected.rawPeak == 1600000);

  assert(smoothMicLevel(0, 1000, 20) == 1000);
  assert(smoothMicLevel(1000, 2000, 20) == 1200);
  assert(smoothMicLevel(1000, 2000, 0) == 1000);
  assert(smoothMicLevel(1000, 2000, 100) == 2000);

  assert(noisePercentFor(0, 1000) == 0);
  assert(noisePercentFor(500, 1000) == 50);
  assert(noisePercentFor(1500, 1000) == 100);
  assert(noisePercentFor(1000, 0) == 0);

  return 0;
}
