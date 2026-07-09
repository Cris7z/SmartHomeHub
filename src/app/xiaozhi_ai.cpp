#include "xiaozhi_ai.h"

#include <Arduino.h>
#include <cstring>

#include "event_log.h"
#include "hub_state.h"
#include "xiaozhi_core.h"
#include "../board/config.h"
#include "../board/hardware.h"
#include "../net/secrets_config.h"
#include "../net/voice_stream_client.h"

namespace {
void setXiaozhiPhase(XiaozhiPhase phase, uint32_t nowMs, uint32_t holdMs) {
  state.xiaozhiPhase = (uint8_t)phase;
  state.xiaozhiPhaseUntilMs = holdMs > 0 ? nowMs + holdMs : 0;
  strlcpy(state.xiaozhiStatusText, xiaozhiPhaseName(phase), sizeof(state.xiaozhiStatusText));
}

const char *defaultPrompt() {
  return "Home status request";
}

bool hasText(const char *text) {
  return text && text[0] != '\0';
}

bool lastAutoSoundTriggered = false;
bool autoWakeArmed = false;
uint32_t autoWakeQuietSinceMs = 0;

bool startXiaozhiRelayTurn(const char *source, uint32_t nowMs) {
  if (!voiceStreamReady()) {
    strlcpy(state.xiaozhiErrorText, "Doubao relay offline", sizeof(state.xiaozhiErrorText));
    strlcpy(state.xiaozhiReplyText, state.xiaozhiErrorText, sizeof(state.xiaozhiReplyText));
    queueSpeakerTone(SPEAKER_TONE_HZ, SPEAKER_TONE_MS);
    return false;
  }
  if (!startVoiceStreamTurn()) {
    strlcpy(state.xiaozhiErrorText, "Voice turn busy", sizeof(state.xiaozhiErrorText));
    return false;
  }

  state.xiaozhiLastTriggerMs = nowMs;
  state.xiaozhiMicMutedUntilMs = 0;
  strlcpy(state.xiaozhiPromptText, defaultPrompt(), sizeof(state.xiaozhiPromptText));
  strlcpy(state.xiaozhiReplyText, "Listening...", sizeof(state.xiaozhiReplyText));
  state.xiaozhiErrorText[0] = '\0';
  setXiaozhiPhase(XiaozhiPhase::Listening, nowMs, 0);
  logHubEvent("XIAOZHI relay");
  Serial.printf("[XIAOZHI] relay wake source=%s ready=%d\n",
                source ? source : "AUTO", voiceStreamReady());
  return true;
}
}

void setupXiaozhiAi() {
  state.xiaozhiEnabled = true;
  state.xiaozhiAutoWake = XIAOZHI_AUTO_WAKE_ENABLED;
  state.xiaozhiManualMode = false;
  state.xiaozhiManualRestartMs = 0;
  state.xiaozhiCloudConfigured = false;
  state.doubaoRelayConfigured = voiceStreamConfigured();
  setXiaozhiPhase(XiaozhiPhase::Idle, millis(), 0);
  strlcpy(state.xiaozhiPromptText, "Say XiaoZhi", sizeof(state.xiaozhiPromptText));
  strlcpy(state.xiaozhiReplyText,
          state.doubaoRelayConfigured ? "Doubao relay ready" : "Relay URL not set",
          sizeof(state.xiaozhiReplyText));
  Serial.printf("[XIAOZHI] enabled autoWake=%d doubaoRelay=%d\n",
                state.xiaozhiAutoWake, state.doubaoRelayConfigured);
}

void triggerXiaozhiAi(const char *source) {
  triggerXiaozhiAiWithPrompt(source, nullptr);
}

void toggleXiaozhiAi(const char *source) {
  if (!state.xiaozhiEnabled) {
    return;
  }

  const uint32_t now = millis();
  if (state.xiaozhiManualMode || voiceStreamTurnActive() || state.speakerPlaying) {
    state.xiaozhiManualMode = false;
    state.xiaozhiManualRestartMs = 0;
    cancelVoiceStreamTurn(source ? source : "KEY");
    state.xiaozhiLastTriggerMs = now;
    state.xiaozhiMicMutedUntilMs = now + XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS;
    strlcpy(state.xiaozhiPromptText, "Say XiaoZhi", sizeof(state.xiaozhiPromptText));
    strlcpy(state.xiaozhiReplyText, "Stopped", sizeof(state.xiaozhiReplyText));
    state.xiaozhiErrorText[0] = '\0';
    setXiaozhiPhase(XiaozhiPhase::Idle, now, 0);
    logHubEvent("XIAOZHI stop");
    Serial.printf("[XIAOZHI] toggle stop source=%s\n", source ? source : "KEY");
    return;
  }

  state.xiaozhiManualMode = true;
  state.xiaozhiManualRestartMs = 0;
  if (!startXiaozhiRelayTurn(source, now)) {
    state.xiaozhiManualMode = false;
  }
}

void triggerXiaozhiAiWithPrompt(const char *source, const char *prompt) {
  if (!state.xiaozhiEnabled) {
    return;
  }

  const uint32_t now = millis();
  const char *effectivePrompt = hasText(prompt) ? prompt : defaultPrompt();
  strlcpy(state.xiaozhiPromptText, effectivePrompt, sizeof(state.xiaozhiPromptText));
  if (hasText(prompt)) {
    strlcpy(state.xiaozhiReplyText, "Use board mic for realtime voice",
            sizeof(state.xiaozhiReplyText));
    state.displayPage = 4;
    logHubEvent("XIAOZHI text");
    return;
  }
  startXiaozhiRelayTurn(source, now);
}

void updateXiaozhiAi() {
  const uint32_t now = millis();
  state.speakerPlaying = isSpeakerTonePlaying();
  state.doubaoRelayConfigured = voiceStreamConfigured();
  state.doubaoRelayConnected = voiceStreamReady();
  state.doubaoSessionActive = voiceStreamTurnActive();
  const bool autoSoundRising = state.soundTriggered && !lastAutoSoundTriggered;
  lastAutoSoundTriggered = state.soundTriggered;

  XiaozhiPhase phase = (XiaozhiPhase)state.xiaozhiPhase;
  const bool micSuppressed =
      xiaozhiMicSuppressed(state.speakerPlaying, now, state.xiaozhiMicMutedUntilMs);
  const bool autoWakeReady =
      phase == XiaozhiPhase::Idle &&
      state.doubaoRelayConfigured &&
      state.doubaoRelayConnected &&
      state.wifiStaConnected &&
      !state.securityArmed &&
      !micSuppressed;
  if (!autoWakeReady) {
    autoWakeArmed = false;
    autoWakeQuietSinceMs = 0;
  } else if (!state.soundTriggered) {
    if (autoWakeQuietSinceMs == 0) {
      autoWakeQuietSinceMs = now;
    }
    if (now - autoWakeQuietSinceMs >= XIAOZHI_AUTO_REARM_QUIET_MS) {
      autoWakeArmed = true;
    }
  } else {
    autoWakeQuietSinceMs = 0;
  }

  if (phase == XiaozhiPhase::Idle &&
      state.doubaoRelayConfigured &&
      state.doubaoRelayConnected &&
      state.wifiStaConnected &&
      autoWakeArmed &&
      xiaozhiShouldAutoTrigger(state.xiaozhiAutoWake, autoSoundRising,
                               micSuppressed,
                               now, state.xiaozhiLastTriggerMs,
                               XIAOZHI_AUTO_WAKE_COOLDOWN_MS)) {
    autoWakeArmed = false;
    startXiaozhiRelayTurn("AUTO mic", now);
    return;
  }

  if (phase == XiaozhiPhase::Speaking) {
    state.speakerPlaying = isSpeakerTonePlaying();
  }
  if (phase != XiaozhiPhase::Idle && !voiceStreamTurnActive() && !state.speakerPlaying) {
    state.xiaozhiMicMutedUntilMs = now + XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS;
    state.xiaozhiManualRestartMs = 0;
    setXiaozhiPhase(XiaozhiPhase::Idle, now, 0);
    phase = XiaozhiPhase::Idle;
  }

  state.xiaozhiManualMode = false;
  state.xiaozhiManualRestartMs = 0;
}
