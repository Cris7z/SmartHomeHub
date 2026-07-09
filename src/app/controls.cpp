#include "controls.h"

#include <Arduino.h>

#include "event_log.h"
#include "hub_state.h"
#include "shortcut_macro.h"
#include "xiaozhi_ai.h"
#include "../board/config.h"

namespace {
constexpr uint8_t DISPLAY_PAGE_COUNT = 5;

void applyShortcutMacro(ShortcutMacroKind kind, const char *label) {
  const ShortcutMacroPreset preset = shortcutMacroPreset(kind);

  state.forceSecurity = preset.forceSecurity;
  state.alarm = preset.alarm;
  state.lampOverride = preset.lampOverride;
  state.manualLamp = preset.manualLamp;
  state.manualAc = preset.manualAc;
  state.acCommandRequested = false;
  state.irTestActive = false;
  state.irTestUntilMs = 0;
  state.lastIrTestBurstMs = 0;
  state.displayPage = preset.displayPage;

  logHubEvent(preset.eventText);
  Serial.printf(
    "[%s] Macro %s: security=%s lamp=%s page=%u\n",
    label,
    preset.serialName,
    state.forceSecurity ? "ON" : "AUTO",
    state.lampOverride ? (state.manualLamp ? "ON" : "OFF") : "AUTO",
    state.displayPage
  );
}
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
    case HubCommand::RunMacroHome:
      applyShortcutMacro(ShortcutMacroKind::Home, label);
      break;
    case HubCommand::RunMacroAway:
      applyShortcutMacro(ShortcutMacroKind::Away, label);
      break;
    case HubCommand::RunMacroNight:
      applyShortcutMacro(ShortcutMacroKind::Night, label);
      break;
    case HubCommand::TriggerXiaozhi:
      triggerXiaozhiAi(label);
      break;
    case HubCommand::ToggleXiaozhi:
      toggleXiaozhiAi(label);
      break;
    case HubCommand::SetSecurityOn:
      state.forceSecurity = true;
      logHubEvent("AI SEC on");
      Serial.printf("[%s] AI security: ON\n", label);
      break;
    case HubCommand::SetSecurityOff:
      state.forceSecurity = false;
      logHubEvent("AI SEC off");
      Serial.printf("[%s] AI security: OFF\n", label);
      break;
    case HubCommand::SetLampOn:
      state.lampOverride = true;
      state.manualLamp = true;
      logHubEvent("AI LAMP on");
      Serial.printf("[%s] AI lamp: ON\n", label);
      break;
    case HubCommand::SetLampOff:
      state.lampOverride = true;
      state.manualLamp = false;
      logHubEvent("AI LAMP off");
      Serial.printf("[%s] AI lamp: OFF\n", label);
      break;
    case HubCommand::SetAcOn:
      state.manualAc = true;
      state.acCommandRequested = true;
      state.irTestActive = true;
      state.irTestUntilMs = millis() + IR_TEST_WINDOW_MS;
      state.lastIrTestBurstMs = 0;
      logHubEvent("AI AC on");
      Serial.printf("[%s] AI AC: ON\n", label);
      break;
    case HubCommand::SetAcOff:
      state.manualAc = false;
      state.acCommandRequested = false;
      logHubEvent("AI AC off");
      Serial.printf("[%s] AI AC: OFF\n", label);
      break;
  }
}
