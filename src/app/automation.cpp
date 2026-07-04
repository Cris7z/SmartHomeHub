#include "automation.h"

#include <Arduino.h>

#include "alarm_logic.h"
#include "event_log.h"
#include "hub_state.h"
#include "../board/config.h"
#include "../board/hardware.h"
#include "../net/pushplus.h"

void updateAutomation() {
  const uint32_t now = millis();
  static bool lastAcCooling = false;
  const bool roomEmptyLongEnough = now - state.lastSeenMs > EMPTY_TO_SECURITY_MS;
  state.securityArmed = state.forceSecurity || (!state.presence && roomEmptyLongEnough);

  const AlarmStateUpdate alarmUpdate = updateAlarmState(
      state.securityArmed, state.soundTriggered, state.alarm, state.alarmUntilMs, now, ALARM_HOLD_MS);
  state.alarm = alarmUpdate.alarm;
  state.alarmUntilMs = alarmUpdate.alarmUntilMs;
  if (alarmUpdate.justTriggered) {
    logHubEvent("ALARM noise");
    if (state.lastPhoneAlertMs == 0 || now - state.lastPhoneAlertMs > PHONE_ALERT_COOLDOWN_MS) {
      state.lastPhoneAlertMs = now;
      sendPhoneAlert();
    }
  }
  if (alarmUpdate.justCleared) {
    logHubEvent("ALARM clear");
    Serial.println("[ALARM] quiet for 5s, cleared");
  }

  state.acCooling = state.manualAc || state.tempC >= TEMP_COOLING_THRESHOLD_C;
  if (state.acCooling != lastAcCooling) {
    lastAcCooling = state.acCooling;
    logHubEvent(state.acCooling ? "AC cooling" : "AC standby");
  }
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

  const bool lampOn = state.lampOverride ? state.manualLamp : state.presence;
  buzzerWrite(state.alarm || now < state.buzzerTestUntilMs);
  setLamp(lampOn, state.alarm);
}
