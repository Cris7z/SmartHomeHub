#include "diagnostics.h"

#include <Arduino.h>

#include "hub_state.h"
#include "xiaozhi_core.h"
#include "../io/mic_processing.h"

void printSerialStatus() {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < 2000) return;
  const uint32_t now = millis();
  lastPrintMs = now;
  const bool xiaozhiMicMuted =
      state.speakerPlaying ||
      (state.xiaozhiMicMutedUntilMs != 0 && now < state.xiaozhiMicMutedUntilMs);
  const bool lampActuallyOn = state.alarm || (state.lampOverride ? state.manualLamp : state.presence);

  Serial.printf("T=%.1fC H=%.0f%% presence=%d security=%d sound=%d mic=%ld micPct=%d%% base=%ld thr=%ld risk=%u/%s ir=%d alarm=%d ac=%d lamp=%d lampOverride=%d page=%u xz=%s xzMute=%d relay=%d/%d/%d spk=%d play=%d ble=%d sta=%d ip=%s push=%d time=%s weather=%s out=%.1fC loc=%s rise=%s set=%s\n",
                state.tempC, state.humidity, state.presence, state.securityArmed,
                state.soundTriggered, (long)state.micLevel,
                noisePercentFor(state.micLevel, state.micThreshold), (long)state.micBaseline,
                (long)state.micThreshold, state.aiRiskScore, state.aiRiskText,
                state.irReceived, state.alarm, state.acCooling,
                lampActuallyOn, state.lampOverride, state.displayPage,
                xiaozhiPhaseName((XiaozhiPhase)state.xiaozhiPhase),
                xiaozhiMicMuted, state.doubaoRelayConfigured,
                state.doubaoRelayConnected, state.doubaoSessionActive,
                state.i2sSpeakerOk, state.speakerPlaying,
                state.bleClientConnected, state.wifiStaConnected,
                state.ipText, state.pushPlusConfigured, state.timeReady ? state.timeText : "--:--",
                state.weatherReady ? state.weatherText : "--", state.outdoorTempC,
                state.locationText, state.sunriseText, state.sunsetText);
}
