#include "xiaozhi_ai.h"

#include <Arduino.h>
#include <cstring>

#include "event_log.h"
#include "hub_state.h"
#include "xiaozhi_core.h"
#include "../board/config.h"
#include "../board/hardware.h"
#include "../board/xiaozhi_voice_clip.h"
#include "../net/secrets_config.h"
#include "../net/xiaozhi_cloud.h"

namespace {
void setXiaozhiPhase(XiaozhiPhase phase, uint32_t nowMs, uint32_t holdMs) {
  state.xiaozhiPhase = (uint8_t)phase;
  state.xiaozhiPhaseUntilMs = holdMs > 0 ? nowMs + holdMs : 0;
  strlcpy(state.xiaozhiStatusText, xiaozhiPhaseName(phase), sizeof(state.xiaozhiStatusText));
}

void buildLocalPrompt() {
  strlcpy(state.xiaozhiPromptText, "Home status request", sizeof(state.xiaozhiPromptText));
  formatXiaozhiLocalReply(state.xiaozhiReplyText, sizeof(state.xiaozhiReplyText),
                          state.tempC, state.humidity,
                          state.presence, state.securityArmed);
}

bool hasXiaozhiCloudConfig() {
  return isXiaozhiCloudConfigured();
}

void startThinking(uint32_t nowMs) {
  buildLocalPrompt();
  setXiaozhiPhase(XiaozhiPhase::Thinking, nowMs, XIAOZHI_THINK_MS);
  logHubEvent("XIAOZHI think");
  bool cloudOk = false;
  if (state.xiaozhiCloudConfigured) {
    cloudOk = requestXiaozhiCloudReply(state.xiaozhiReplyText,
                                       sizeof(state.xiaozhiReplyText),
                                       state.xiaozhiPromptText,
                                       state.tempC, state.humidity,
                                       state.presence, state.securityArmed,
                                       state.weatherText, state.timeText);
  }
  if (!cloudOk) {
    formatXiaozhiLocalReply(state.xiaozhiReplyText, sizeof(state.xiaozhiReplyText),
                            state.tempC, state.humidity,
                            state.presence, state.securityArmed);
  }
  Serial.printf("[XIAOZHI] prompt='%s' replySource=%s\n",
                state.xiaozhiPromptText, cloudOk ? "CLOUD" : "LOCAL");
}

void startSpeaking(uint32_t nowMs) {
  setXiaozhiPhase(XiaozhiPhase::Speaking, nowMs, XIAOZHI_SPEAK_MS);
  state.xiaozhiMicMutedUntilMs = 0;
  const bool voiceQueued = queueSpeakerPcmClip(XIAOZHI_HELLO_PCM, XIAOZHI_HELLO_PCM_SAMPLE_COUNT);
  const bool toneQueued = voiceQueued ? false : queueSpeakerTone(SPEAKER_TONE_HZ, SPEAKER_TONE_MS);
  state.speakerPlaying = voiceQueued || toneQueued;
  logHubEvent("XIAOZHI reply");
  Serial.printf("[XIAOZHI] reply='%s' speaker=%s cloud=%s\n",
                state.xiaozhiReplyText,
                voiceQueued ? "VOICE" : (toneQueued ? "TONE" : "SKIP"),
                state.xiaozhiCloudConfigured ? "CONFIGURED" : "LOCAL");
  if (state.speakerPlaying) {
    Serial.printf("[XIAOZHI] mic muted while speaker playing\n");
  }
}
}

void setupXiaozhiAi() {
  state.xiaozhiEnabled = true;
  state.xiaozhiAutoWake = XIAOZHI_AUTO_WAKE_ENABLED;
  state.xiaozhiCloudConfigured = hasXiaozhiCloudConfig();
  setXiaozhiPhase(XiaozhiPhase::Idle, millis(), 0);
  strlcpy(state.xiaozhiPromptText, "Say XiaoZhi", sizeof(state.xiaozhiPromptText));
  strlcpy(state.xiaozhiReplyText,
          state.xiaozhiCloudConfigured ? "Cloud endpoint ready" : "Local demo ready",
          sizeof(state.xiaozhiReplyText));
  Serial.printf("[XIAOZHI] enabled autoWake=%d cloud=%d\n",
                state.xiaozhiAutoWake, state.xiaozhiCloudConfigured);
}

void triggerXiaozhiAi(const char *source) {
  if (!state.xiaozhiEnabled) {
    return;
  }

  const uint32_t now = millis();
  state.xiaozhiLastTriggerMs = now;
  state.xiaozhiMicMutedUntilMs = 0;
  strlcpy(state.xiaozhiPromptText, source ? source : "manual", sizeof(state.xiaozhiPromptText));
  strlcpy(state.xiaozhiReplyText, "Listening...", sizeof(state.xiaozhiReplyText));
  setXiaozhiPhase(XiaozhiPhase::Listening, now, XIAOZHI_LISTEN_MS);
  state.displayPage = 4;
  logHubEvent("XIAOZHI wake");
  Serial.printf("[XIAOZHI] wake source=%s\n", source ? source : "AUTO");
}

void updateXiaozhiAi() {
  const uint32_t now = millis();
  state.speakerPlaying = isSpeakerTonePlaying();

  const XiaozhiPhase phase = (XiaozhiPhase)state.xiaozhiPhase;
  const bool micSuppressed =
      xiaozhiMicSuppressed(state.speakerPlaying, now, state.xiaozhiMicMutedUntilMs);
  if (phase == XiaozhiPhase::Idle &&
      xiaozhiShouldAutoTrigger(state.xiaozhiAutoWake, state.soundTriggered,
                               micSuppressed,
                               now, state.xiaozhiLastTriggerMs,
                               XIAOZHI_AUTO_WAKE_COOLDOWN_MS)) {
    triggerXiaozhiAi("AUTO mic");
    return;
  }

  if (state.xiaozhiPhaseUntilMs == 0 || now < state.xiaozhiPhaseUntilMs) {
    return;
  }

  switch (phase) {
    case XiaozhiPhase::Listening:
      startThinking(now);
      break;
    case XiaozhiPhase::Thinking:
      startSpeaking(now);
      break;
    case XiaozhiPhase::Speaking:
      if (!isSpeakerTonePlaying()) {
        state.speakerPlaying = false;
        state.xiaozhiMicMutedUntilMs = now + XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS;
        setXiaozhiPhase(XiaozhiPhase::Idle, now, 0);
        Serial.printf("[XIAOZHI] mic wake cooldown %lu ms\n",
                      (unsigned long)XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS);
      }
      break;
    case XiaozhiPhase::Idle:
    default:
      break;
  }
}
