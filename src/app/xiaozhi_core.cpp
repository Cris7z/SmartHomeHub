#include "xiaozhi_core.h"

#include <cstdio>

const char *xiaozhiPhaseName(XiaozhiPhase phase) {
  switch (phase) {
    case XiaozhiPhase::Listening:
      return "LISTEN";
    case XiaozhiPhase::Thinking:
      return "THINK";
    case XiaozhiPhase::Speaking:
      return "SPEAK";
    case XiaozhiPhase::Idle:
    default:
      return "IDLE";
  }
}

bool xiaozhiShouldAutoTrigger(bool enabled, bool soundTriggered,
                              uint32_t nowMs, uint32_t lastTriggerMs,
                              uint32_t cooldownMs) {
  return xiaozhiShouldAutoTrigger(enabled, soundTriggered, false,
                                  nowMs, lastTriggerMs, cooldownMs);
}

bool xiaozhiShouldAutoTrigger(bool enabled, bool soundTriggered,
                              bool micSuppressed,
                              uint32_t nowMs, uint32_t lastTriggerMs,
                              uint32_t cooldownMs) {
  if (!enabled || !soundTriggered || micSuppressed) {
    return false;
  }
  return lastTriggerMs == 0 || nowMs - lastTriggerMs >= cooldownMs;
}

bool xiaozhiMicSuppressed(bool speakerPlaying,
                          uint32_t nowMs,
                          uint32_t mutedUntilMs) {
  return speakerPlaying || (mutedUntilMs != 0 && nowMs < mutedUntilMs);
}

void formatXiaozhiLocalReply(char *out, size_t outSize, float tempC,
                             float humidity, bool presence,
                             bool securityArmed) {
  if (!out || outSize == 0) {
    return;
  }

  std::snprintf(out, outSize, "Home %.1fC %.0f%% %s %s",
                tempC, humidity,
                presence ? "OCCUPIED" : "EMPTY",
                securityArmed ? "SEC ARMED" : "SEC OFF");
}
