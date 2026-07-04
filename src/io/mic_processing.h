#pragma once

#include <cstdint>

struct MicAnalysis {
  int32_t rawPeak = 0;
  int32_t acPeak = 0;
  int32_t rms = 0;
  int32_t dcMean = 0;
  int sampleCount = 0;
  bool valid = false;
};

MicAnalysis analyzeI2sMicSamples(const int32_t *samples, int count, int sampleShift);
int32_t smoothMicLevel(int32_t previous, int32_t current, int newWeightPercent);
int noisePercentFor(int32_t level, int32_t threshold);
