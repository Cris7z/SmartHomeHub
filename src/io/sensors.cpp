#include "sensors.h"

#include <Arduino.h>
#include <driver/i2s.h>

#include "../app/hub_state.h"
#include "../board/config.h"
#include "../board/hardware.h"
#include "mic_processing.h"

namespace {
uint32_t lastSensorReadMs = 0;
uint32_t lastMicReadMs = 0;
uint32_t lastMicDebugMs = 0;
int32_t smoothedMicLevel = 0;

int32_t updateAdaptiveMicThreshold(int32_t level) {
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

void readEnvironment() {
  if (millis() - lastSensorReadMs < 2000) return;
  lastSensorReadMs = millis();

  if (state.ahtOk) {
    sensors_event_t humidityEvent;
    sensors_event_t tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);
    state.tempC = tempEvent.temperature;
    state.humidity = humidityEvent.relative_humidity;
  } else {
    // Keep the interface alive when the sensor is not connected yet.
    state.tempC = 26.0 + 2.0 * sin(millis() / 9000.0);
    state.humidity = 55.0 + 8.0 * sin(millis() / 13000.0);
  }
}

void readMicrophone() {
  if (!state.i2sOk) {
    state.micLevel = 0;
    smoothedMicLevel = 0;
    state.soundTriggered = false;
    return;
  }

  if (millis() - lastMicReadMs < 120) return;
  lastMicReadMs = millis();

  int32_t samples[256];
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead, pdMS_TO_TICKS(20));
  if (result != ESP_OK || bytesRead == 0) {
    state.micLevel = 0;
    smoothedMicLevel = 0;
    state.soundTriggered = false;
    if (MIC_DEBUG && millis() - lastMicDebugMs > 2000) {
      lastMicDebugMs = millis();
      Serial.printf("[MIC] read err=%d bytes=%u\n", (int)result, (unsigned)bytesRead);
    }
    return;
  }

  const int count = bytesRead / sizeof(samples[0]);
  const MicAnalysis analysis = analyzeI2sMicSamples(samples, count, MIC_SAMPLE_SHIFT);
  smoothedMicLevel = analysis.valid
      ? smoothMicLevel(smoothedMicLevel, analysis.rms, MIC_SMOOTH_WEIGHT_PERCENT)
      : 0;
  state.micLevel = smoothedMicLevel;
  const int32_t threshold = updateAdaptiveMicThreshold(state.micLevel);
  state.soundTriggered = state.micLevel > threshold;
  if (MIC_DEBUG && millis() - lastMicDebugMs > 2000) {
    lastMicDebugMs = millis();
    Serial.printf("[MIC] read err=%d bytes=%u samples=%d used=%d rawPeak=%ld dc=%ld acPeak=%ld rms=%ld smooth=%ld base=%ld thr=%ld pct=%d\n",
                  (int)result, (unsigned)bytesRead, count, analysis.sampleCount,
                  (long)analysis.rawPeak, (long)analysis.dcMean,
                  (long)analysis.acPeak, (long)analysis.rms,
                  (long)state.micLevel, (long)state.micBaseline,
                  (long)threshold, noisePercentFor(state.micLevel, threshold));
  }
}
