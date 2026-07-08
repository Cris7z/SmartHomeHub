#include "diagnostics.h"

#include <Arduino.h>

#include "hub_state.h"

void printSerialStatus() {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < 2000) return;
  lastPrintMs = millis();

  Serial.printf("T=%.1fC H=%.0f%% presence=%d security=%d sound=%d mic=%ld base=%ld thr=%ld risk=%u/%s ir=%d alarm=%d ac=%d lamp=%d lampOverride=%d page=%u ble=%d sta=%d ip=%s push=%d time=%s weather=%s out=%.1fC\n",
                state.tempC, state.humidity, state.presence, state.securityArmed,
                state.soundTriggered, (long)state.micLevel, (long)state.micBaseline,
                (long)state.micThreshold, state.aiRiskScore, state.aiRiskText,
                state.irReceived, state.alarm, state.acCooling,
                state.manualLamp, state.lampOverride, state.displayPage, state.bleClientConnected, state.wifiStaConnected,
                state.ipText, state.pushPlusConfigured, state.timeReady ? state.timeText : "--:--",
                state.weatherReady ? state.weatherText : "--", state.outdoorTempC);
}
