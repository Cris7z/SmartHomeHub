#include "controls.h"

#include <Arduino.h>

#include "event_log.h"
#include "hub_state.h"
#include "../board/config.h"

namespace {
constexpr uint8_t DISPLAY_PAGE_COUNT = 4;
}

void applyHubCommand(HubCommand command, const char *source) {
  const char *label = source ? source : "CTRL";

  switch (command) {
    case HubCommand::ToggleSecurity:
      state.forceSecurity = !state.forceSecurity;
      logHubEvent(state.forceSecurity ? "SEC forced" : "SEC auto");
      Serial.printf("[%s] Force security: %s\n", label, state.forceSecurity ? "ON" : "OFF");
      break;
    case HubCommand::ToggleLamp:
    {
      const bool currentLampOn = state.lampOverride ? state.manualLamp : state.presence;
      state.lampOverride = true;
      state.manualLamp = !currentLampOn;
      logHubEvent(state.manualLamp ? "LAMP manual on" : "LAMP manual off");
      Serial.printf("[%s] Manual lamp override: %s\n", label, state.manualLamp ? "ON" : "OFF");
      break;
    }
    case HubCommand::ToggleAc:
      state.manualAc = !state.manualAc;
      state.irTestActive = true;
      state.irTestUntilMs = millis() + IR_TEST_WINDOW_MS;
      state.lastIrTestBurstMs = 0;
      if (state.manualAc || state.tempC >= TEMP_COOLING_THRESHOLD_C) {
        state.acCommandRequested = true;
      }
      logHubEvent(state.manualAc ? "AC manual on" : "AC manual off");
      Serial.printf("[%s] Manual AC: %s\n", label, state.manualAc ? "ON" : "OFF");
      Serial.printf("[IR] Diagnostic test started: %lu ms\n", IR_TEST_WINDOW_MS);
      break;
    case HubCommand::NextDisplayPage:
      state.displayPage = (state.displayPage + 1) % DISPLAY_PAGE_COUNT;
      Serial.printf("[%s] Display page: %u\n", label, state.displayPage);
      break;
    case HubCommand::ClearAlarmSecurity:
      state.alarm = false;
      state.forceSecurity = false;
      state.lampOverride = false;
      logHubEvent("CLEAR alarm");
      Serial.printf("[%s] Alarm/security cleared\n", label);
      break;
  }
}
