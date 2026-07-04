#include "inputs.h"

#include <Arduino.h>

#include "../app/hub_state.h"
#include "../board/config.h"
#include "radar_filter.h"
#include "sensors.h"

namespace {
constexpr int BUTTON_COUNT = 5;
const int BUTTON_PINS[BUTTON_COUNT] = {
  PIN_BUTTON_1,
  PIN_BUTTON_2,
  PIN_BUTTON_3,
  PIN_BUTTON_4,
  PIN_BUTTON_5
};

int lastButtonLevel[BUTTON_COUNT] = {
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL
};
uint32_t lastButtonChangeMs[BUTTON_COUNT] = {0, 0, 0, 0, 0};
volatile bool irPulseSeen = false;
RadarPresenceFilter radarFilter;
bool lastLoggedPresence = false;

void IRAM_ATTR handleIrPulse() {
  irPulseSeen = true;
}

void handleButtonPress(int index) {
  switch (index) {
    case 0:
      state.forceSecurity = !state.forceSecurity;
      Serial.printf("[KEY1] Force security: %s\n", state.forceSecurity ? "ON" : "OFF");
      break;
    case 1:
      state.manualLamp = !state.manualLamp;
      Serial.printf("[KEY2] Manual lamp: %s\n", state.manualLamp ? "ON" : "OFF");
      break;
    case 2:
      state.manualAc = !state.manualAc;
      state.irTestActive = true;
      state.irTestUntilMs = millis() + IR_TEST_WINDOW_MS;
      state.lastIrTestBurstMs = 0;
      if (state.manualAc || state.tempC >= TEMP_COOLING_THRESHOLD_C) {
        state.acCommandRequested = true;
      }
      Serial.printf("[KEY3] Manual AC: %s\n", state.manualAc ? "ON" : "OFF");
      Serial.printf("[IR] Diagnostic test started: %lu ms\n", IR_TEST_WINDOW_MS);
      break;
    case 3:
      state.buzzerTestUntilMs = millis() + 300;
      Serial.println("[KEY4] Buzzer test");
      break;
    case 4:
      state.alarm = false;
      state.forceSecurity = false;
      Serial.println("[KEY5] Alarm/security cleared");
      break;
  }
}
}

void setupInputs() {
  pinMode(PIN_IR_RX, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_IR_RX), handleIrPulse, FALLING);
  pinMode(PIN_PRESENCE_OUT, INPUT_PULLUP);
  for (int i = 0; i < BUTTON_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLDOWN);
  }
}

void readInputs() {
  const uint32_t now = millis();
  const bool rawPresence = digitalRead(PIN_PRESENCE_OUT) == PRESENCE_ACTIVE_LEVEL;
  state.presence = radarFilter.update(rawPresence, now);
  if (state.presence != lastLoggedPresence) {
    lastLoggedPresence = state.presence;
    Serial.printf("[RADAR] %s\n", state.presence ? "occupied" : "empty");
  }
  const bool irLevelActive = digitalRead(PIN_IR_RX) == IR_RX_ACTIVE_LEVEL;
  bool irPulseCaptured = false;

  noInterrupts();
  if (irPulseSeen) {
    irPulseSeen = false;
    irPulseCaptured = true;
  }
  interrupts();

  if (irLevelActive || irPulseCaptured) {
    if (!state.irReceived) {
      Serial.println("[IR] RX pulse latched");
    }
    state.irReceivedUntilMs = now + IR_RX_HOLD_MS;
  }
  state.irReceived = now < state.irReceivedUntilMs;
  readMicrophone();

  if (state.presence) {
    state.lastSeenMs = now;
  }

  for (int i = 0; i < BUTTON_COUNT; i++) {
    int buttonLevel = digitalRead(BUTTON_PINS[i]);
    if (buttonLevel != lastButtonLevel[i] && now - lastButtonChangeMs[i] > 50) {
      lastButtonChangeMs[i] = now;
      lastButtonLevel[i] = buttonLevel;
      if (buttonLevel == BUTTON_ACTIVE_LEVEL) {
        handleButtonPress(i);
      }
    }
  }
}
