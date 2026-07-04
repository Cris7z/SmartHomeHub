#include "mic_processing.h"

#include <cmath>

namespace {
int32_t abs32Safe(int32_t value) {
  if (value == INT32_MIN) return INT32_MAX;
  return value < 0 ? -value : value;
}

int32_t clampPercent(int32_t value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return value;
}

MicAnalysis analyzeChannel(const int32_t *samples, int count, int sampleShift,
                           int startIndex, int stride) {
  MicAnalysis analysis;
  if (!samples || count <= 0 || stride <= 0 || startIndex < 0 || startIndex >= count) {
    return analysis;
  }

  int64_t sum = 0;
  for (int i = startIndex; i < count; i += stride) {
    const int32_t rawMagnitude = abs32Safe(samples[i]);
    if (rawMagnitude > analysis.rawPeak) analysis.rawPeak = rawMagnitude;
    sum += samples[i] >> sampleShift;
    analysis.sampleCount++;
  }

  if (analysis.sampleCount <= 0) {
    return analysis;
  }

  analysis.dcMean = (int32_t)(sum / analysis.sampleCount);

  int64_t sumSquares = 0;
  for (int i = startIndex; i < count; i += stride) {
    const int32_t centered = (samples[i] >> sampleShift) - analysis.dcMean;
    const int32_t magnitude = abs32Safe(centered);
    if (magnitude > analysis.acPeak) analysis.acPeak = magnitude;
    sumSquares += (int64_t)centered * centered;
  }

  analysis.rms = (int32_t)std::sqrt((double)sumSquares / analysis.sampleCount);
  analysis.valid = true;
  return analysis;
}

MicAnalysis louderAnalysis(const MicAnalysis &a, const MicAnalysis &b) {
  if (!a.valid) return b;
  if (!b.valid) return a;
  if (b.rms > a.rms) return b;
  if (b.rms == a.rms && b.rawPeak > a.rawPeak) return b;
  return a;
}
}

MicAnalysis analyzeI2sMicSamples(const int32_t *samples, int count, int sampleShift) {
  if (!samples || count <= 0 || sampleShift < 0 || sampleShift >= 31) {
    return MicAnalysis{};
  }

  if (count >= 4) {
    const MicAnalysis evenSlot = analyzeChannel(samples, count, sampleShift, 0, 2);
    const MicAnalysis oddSlot = analyzeChannel(samples, count, sampleShift, 1, 2);
    return louderAnalysis(evenSlot, oddSlot);
  }

  return analyzeChannel(samples, count, sampleShift, 0, 1);
}

int32_t smoothMicLevel(int32_t previous, int32_t current, int newWeightPercent) {
  if (current < 0) current = 0;
  if (previous <= 0) return current;
  if (newWeightPercent <= 0) return previous;
  if (newWeightPercent >= 100) return current;

  return (previous * (100 - newWeightPercent) + current * newWeightPercent) / 100;
}

int noisePercentFor(int32_t level, int32_t threshold) {
  if (level <= 0 || threshold <= 0) return 0;
  const int64_t scaled = ((int64_t)level * 100 + threshold / 2) / threshold;
  return clampPercent((int32_t)scaled);
}
