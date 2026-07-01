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
  if (millis() - lastMicReadMs < 120) return;
  lastMicReadMs = millis();

  int32_t samples[128];
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead, 2);
  if (result != ESP_OK || bytesRead == 0) {
    state.micLevel = 0;
    state.soundTriggered = false;
    return;
  }

  const int count = bytesRead / sizeof(samples[0]);
  int64_t sumSquares = 0;
  for (int i = 0; i < count; i++) {
    int32_t sample = samples[i] >> 14;
    sumSquares += (int64_t)sample * sample;
  }

  state.micLevel = sqrt((double)sumSquares / count);
  state.soundTriggered = state.micLevel > MIC_RMS_TRIGGER;
}
