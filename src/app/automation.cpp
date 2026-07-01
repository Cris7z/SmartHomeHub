#include "automation.h"

#include <Arduino.h>

#include "hub_state.h"
#include "../board/config.h"
#include "../board/hardware.h"

void updateAutomation() {
  const bool roomEmptyLongEnough = millis() - state.lastSeenMs > EMPTY_TO_SECURITY_MS;
  state.securityArmed = state.forceSecurity || (!state.presence && roomEmptyLongEnough);

  if (state.securityArmed && state.soundTriggered) {
    state.alarm = true;
    state.alarmUntilMs = millis() + ALARM_HOLD_MS;
  }
  if (state.alarm && millis() > state.alarmUntilMs) {
    state.alarm = false;
  }

  state.acCooling = state.manualAc || state.tempC >= TEMP_COOLING_THRESHOLD_C;
  if (state.acCooling && millis() - state.lastAcCommandMs > AC_COMMAND_GAP_MS) {
    state.lastAcCommandMs = millis();
    sendIrDemoBurst();
    Serial.println("[AC] Demo IR cooling command sent");
  }

  buzzerWrite(state.alarm || millis() < state.buzzerTestUntilMs);
  setLamp(state.presence || state.manualLamp, state.alarm);
}
