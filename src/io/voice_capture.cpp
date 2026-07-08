#include "voice_capture.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "../app/hub_state.h"
#include "../board/config.h"
#include "../board/hardware.h"
#include "mic_processing.h"
#include "voice_capture_core.h"

namespace {
int16_t *preRollStorage = nullptr;
VoicePreRoll preRoll(nullptr, 0);
bool captureReady = false;
bool turnActive = false;
int lockedSlot = -1;
int32_t smoothedVoiceMicLevel = 0;

int32_t updateVoiceMicThreshold(int32_t level) {
  if (level <= 0) {
    state.micThreshold = MIC_RMS_TRIGGER;
    return state.micThreshold;
  }
  if (!state.adaptiveMicReady) {
    state.adaptiveMicReady = true;
    state.micBaseline = level;
  }
  const int32_t currentThreshold = state.micThreshold > 0 ? state.micThreshold : MIC_RMS_TRIGGER;
  if (!state.alarm && level < currentThreshold) {
    state.micBaseline = (state.micBaseline * 95 + level * 5) / 100;
  }
  const int32_t adaptiveThreshold =
      state.micBaseline * MIC_RMS_ADAPT_MULTIPLIER_PERCENT / 100 + MIC_RMS_ADAPT_MARGIN;
  state.micThreshold = max((int32_t)MIC_RMS_TRIGGER, adaptiveThreshold);
  return state.micThreshold;
}
}

bool setupVoiceCapture() {
  if (!state.i2sOk) {
    captureReady = false;
    return false;
  }
  if (!preRollStorage) {
    preRollStorage = static_cast<int16_t *>(heap_caps_malloc(
        VOICE_PREROLL_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!preRollStorage) {
      preRollStorage = static_cast<int16_t *>(
          heap_caps_malloc(VOICE_PREROLL_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT));
    }
  }
  if (!preRollStorage) {
    captureReady = false;
    return false;
  }
  preRoll = VoicePreRoll(preRollStorage, VOICE_PREROLL_SAMPLES);
  captureReady = true;
  return true;
}

bool pollVoiceCapturePacket(int16_t *out, size_t outCapacity, size_t &sampleCount) {
  sampleCount = 0;
  if (!captureReady) {
    return false;
  }

  int32_t raw[VOICE_PACKET_SAMPLES * 2] = {};
  size_t rawCount = 0;
  if (!readI2sMicFrames(raw, sizeof(raw) / sizeof(raw[0]), rawCount, 20) || rawCount == 0) {
    state.micLevel = 0;
    smoothedVoiceMicLevel = 0;
    state.soundTriggered = false;
    return false;
  }

  const MicAnalysis analysis = analyzeI2sMicSamples(raw, (int)rawCount, MIC_SAMPLE_SHIFT);
  smoothedVoiceMicLevel = analysis.valid
      ? smoothMicLevel(smoothedVoiceMicLevel, analysis.rms, MIC_SMOOTH_WEIGHT_PERCENT)
      : 0;
  state.micLevel = smoothedVoiceMicLevel;
  const int32_t threshold = updateVoiceMicThreshold(state.micLevel);
  state.soundTriggered = state.micLevel > threshold;

  if (turnActive && lockedSlot < 0) {
    lockedSlot = selectVoiceI2sSlot(raw, rawCount);
  }
  const uint8_t slot = lockedSlot >= 0 ? (uint8_t)lockedSlot : (uint8_t)selectVoiceI2sSlot(raw, rawCount);

  int16_t converted[VOICE_PACKET_SAMPLES] = {};
  const size_t convertedCount = convertVoiceI2sToPcm16(
      raw, rawCount, slot, VOICE_PCM_RIGHT_SHIFT, converted, VOICE_PACKET_SAMPLES);
  if (convertedCount > 0) {
    preRoll.push(converted, convertedCount);
  }

  if (!turnActive) {
    return true;
  }
  if (!out || outCapacity < VOICE_PACKET_SAMPLES || convertedCount != VOICE_PACKET_SAMPLES) {
    return false;
  }
  for (size_t i = 0; i < convertedCount; ++i) {
    out[i] = converted[i];
  }
  sampleCount = convertedCount;
  return true;
}

size_t copyVoicePreRoll(int16_t *out, size_t outCapacity) {
  return preRoll.copyChronological(out, outCapacity);
}

void beginVoiceCaptureTurn() {
  if (!captureReady) {
    return;
  }
  turnActive = true;
  lockedSlot = -1;
}

void endVoiceCaptureTurn() {
  turnActive = false;
  lockedSlot = -1;
}

bool voiceCaptureOwnsMic() {
  return captureReady;
}
