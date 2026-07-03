#include "inputs.h"

#include <Arduino.h>

#include "../app/controls.h"
#include "../app/event_log.h"
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
volatile bool irPulseSeen = false;
bool presenceKnown = false;
bool lastPresence = false;

void IRAM_ATTR handleIrPulse() {
  irPulseSeen = true;
}

void handleButtonPress(int index) {
  switch (index) {
    case 0:
      applyHubCommand(HubCommand::ToggleSecurity, "KEY1");
      break;
    case 1:
      applyHubCommand(HubCommand::ToggleLamp, "KEY2");
      break;
    case 2:
      applyHubCommand(HubCommand::ToggleAc, "KEY3");
      break;
    case 3:
      applyHubCommand(HubCommand::NextDisplayPage, "KEY4");
      break;
    case 4:
      applyHubCommand(HubCommand::ClearAlarmSecurity, "KEY5");
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
  state.presence = digitalRead(PIN_PRESENCE_OUT) == PRESENCE_ACTIVE_LEVEL;
  if (!presenceKnown) {
    presenceKnown = true;
    lastPresence = state.presence;
  } else if (state.presence != lastPresence) {
    lastPresence = state.presence;
    logHubEvent(state.presence ? "ROOM occupied" : "ROOM empty");
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
