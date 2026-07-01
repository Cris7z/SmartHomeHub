#include "automation.h"

#include <Arduino.h>

#include "hub_state.h"
#include "../board/config.h"
#include "../board/hardware.h"

void updateAutomation() {
  const uint32_t now = millis();
  const bool roomEmptyLongEnough = now - state.lastSeenMs > EMPTY_TO_SECURITY_MS;
  state.securityArmed = state.forceSecurity || (!state.presence && roomEmptyLongEnough);

  if (state.securityArmed && state.soundTriggered) {
    state.alarm = true;
    state.alarmUntilMs = now + ALARM_HOLD_MS;
  }
  if (state.alarm && now > state.alarmUntilMs) {
    state.alarm = false;
  }

  state.acCooling = state.manualAc || state.tempC >= TEMP_COOLING_THRESHOLD_C;
  if (!state.acCooling) {
    state.acCommandRequested = false;
  }

  if (state.acCooling && (state.acCommandRequested || now - state.lastAcCommandMs > AC_COMMAND_GAP_MS)) {
    state.acCommandRequested = false;
    state.lastAcCommandMs = now;
    sendIrDemoBurst();
    Serial.println("[AC] Demo IR cooling command sent");
  }

  if (state.irTestActive) {
    if (now < state.irTestUntilMs) {
      if (state.lastIrTestBurstMs == 0 || now - state.lastIrTestBurstMs >= IR_TEST_BURST_GAP_MS) {
        state.lastIrTestBurstMs = now;
        sendIrDemoBurst();
        Serial.println("[IR] Diagnostic burst");
      }
    } else {
      state.irTestActive = false;
      state.irTestUntilMs = 0;
      Serial.println("[IR] Diagnostic test ended");
    }
  }

  buzzerWrite(state.alarm || now < state.buzzerTestUntilMs);
  setLamp(state.presence || state.manualLamp, state.alarm);
}
