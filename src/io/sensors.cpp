#include "sensors.h"

#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>

#include "../app/hub_state.h"
#include "../board/config.h"
#include "../board/hardware.h"

namespace {
uint32_t lastSensorReadMs = 0;
uint32_t lastMicReadMs = 0;
uint32_t lastMicDebugMs = 0;

int32_t abs32(int32_t value) {
  return value < 0 ? -value : value;
}

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
    state.soundTriggered = false;
    if (MIC_DEBUG && millis() - lastMicDebugMs > 2000) {
      lastMicDebugMs = millis();
      Serial.printf("[MIC] read err=%d bytes=%u\n", (int)result, (unsigned)bytesRead);
    }
    return;
  }

  const int count = bytesRead / sizeof(samples[0]);
  int64_t sumSquares = 0;
  int nonzeroCount = 0;
  int32_t peak = 0;
  int32_t rawPeak = 0;
  for (int i = 0; i < count; i++) {
    int32_t rawMagnitude = abs32(samples[i]);
    if (rawMagnitude > rawPeak) rawPeak = rawMagnitude;
    int32_t sample = samples[i] >> MIC_SAMPLE_SHIFT;
    int32_t magnitude = abs32(sample);
    if (magnitude > 0) nonzeroCount++;
    if (magnitude > peak) peak = magnitude;
    sumSquares += (int64_t)sample * sample;
  }

  state.micLevel = sqrt((double)sumSquares / count);
  const int32_t threshold = updateAdaptiveMicThreshold(state.micLevel);
  state.soundTriggered = state.micLevel > threshold;
  if (MIC_DEBUG && millis() - lastMicDebugMs > 2000) {
    lastMicDebugMs = millis();
    Serial.printf("[MIC] read err=%d bytes=%u samples=%d nonzero=%d rawPeak=%ld peak=%ld rms=%ld base=%ld thr=%ld\n",
                  (int)result, (unsigned)bytesRead, count, nonzeroCount,
                  (long)rawPeak, (long)peak, (long)state.micLevel,
                  (long)state.micBaseline, (long)threshold);
  }
}
