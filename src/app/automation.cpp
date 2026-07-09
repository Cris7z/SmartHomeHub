#include "automation.h"

#include <Arduino.h>

#include "alarm_logic.h"
#include "event_log.h"
#include "hub_state.h"
#include "../board/config.h"
#include "../board/hardware.h"
#include "../net/pushplus.h"
#include "../net/voice_stream_client.h"

void updateAutomation() {
  const uint32_t now = millis();
  static bool lastAcCooling = false;
  static bool autoAcCooling = false;
  static bool lastSecurityArmed = false;
  static uint32_t armedSinceMs = 0;
  static uint32_t loudSinceMs = 0;
  const bool roomEmptyLongEnough = now - state.lastSeenMs > EMPTY_TO_SECURITY_MS;
  state.securityArmed = state.forceSecurity || (!state.presence && roomEmptyLongEnough);

  if (state.securityArmed != lastSecurityArmed) {
    lastSecurityArmed = state.securityArmed;
    armedSinceMs = state.securityArmed ? now : 0;
    loudSinceMs = 0;
  }

  const int32_t alarmNoiseThreshold = max((int32_t)MIC_ALARM_RMS_TRIGGER, state.micThreshold);
  const bool armedGraceDone = state.securityArmed && armedSinceMs != 0 && now - armedSinceMs >= ALARM_ARM_GRACE_MS;
  const bool voiceSessionActive = voiceStreamTurnActive();
  const bool loudEnoughForAlarm =
      armedGraceDone && !voiceSessionActive && state.micLevel > alarmNoiseThreshold;
  if (loudEnoughForAlarm) {
    if (loudSinceMs == 0) {
      loudSinceMs = now;
    }
  } else {
    loudSinceMs = 0;
  }
  const bool alarmSoundConfirmed =
      loudSinceMs != 0 && now - loudSinceMs >= ALARM_SOUND_CONFIRM_MS;

  const AlarmStateUpdate alarmUpdate = updateAlarmState(
      state.securityArmed, alarmSoundConfirmed, state.alarm, state.alarmUntilMs, now, ALARM_HOLD_MS);
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

  if (state.tempC > TEMP_COOLING_THRESHOLD_C) {
    autoAcCooling = true;
  } else if (state.tempC < TEMP_STANDBY_THRESHOLD_C) {
    autoAcCooling = false;
  }
  state.acCooling = state.manualAc || autoAcCooling;
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
