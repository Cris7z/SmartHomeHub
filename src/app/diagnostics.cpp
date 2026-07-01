#include "diagnostics.h"

#include <Arduino.h>

#include "hub_state.h"

void printSerialStatus() {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < 2000) return;
  lastPrintMs = millis();

  Serial.printf("T=%.1fC H=%.0f%% presence=%d security=%d sound=%d mic=%ld ir=%d alarm=%d ac=%d lamp=%d\n",
                state.tempC, state.humidity, state.presence, state.securityArmed,
                state.soundTriggered, (long)state.micLevel, state.irReceived, state.alarm, state.acCooling,
                state.manualLamp);
}
