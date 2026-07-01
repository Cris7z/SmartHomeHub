#include "inputs.h"

#include <Arduino.h>

#include "../app/hub_state.h"
#include "../board/config.h"
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
      Serial.printf("[KEY3] Manual AC: %s\n", state.manualAc ? "ON" : "OFF");
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
  pinMode(PIN_PRESENCE_OUT, INPUT_PULLUP);
  for (int i = 0; i < BUTTON_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT);
  }
}

void readInputs() {
  state.presence = digitalRead(PIN_PRESENCE_OUT) == PRESENCE_ACTIVE_LEVEL;
  state.irReceived = digitalRead(PIN_IR_RX) == IR_RX_ACTIVE_LEVEL;
  readMicrophone();

  if (state.presence) {
    state.lastSeenMs = millis();
  }

  for (int i = 0; i < BUTTON_COUNT; i++) {
    int buttonLevel = digitalRead(BUTTON_PINS[i]);
    if (buttonLevel != lastButtonLevel[i] && millis() - lastButtonChangeMs[i] > 50) {
      lastButtonChangeMs[i] = millis();
      lastButtonLevel[i] = buttonLevel;
      if (buttonLevel == BUTTON_ACTIVE_LEVEL) {
        handleButtonPress(i);
      }
    }
  }
}
