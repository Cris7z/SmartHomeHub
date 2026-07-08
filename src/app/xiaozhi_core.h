#pragma once

#include <cstddef>
#include <cstdint>

enum class XiaozhiPhase : uint8_t {
  Idle = 0,
  Listening,
  Thinking,
  Speaking
};

const char *xiaozhiPhaseName(XiaozhiPhase phase);
bool xiaozhiShouldAutoTrigger(bool enabled, bool soundTriggered,
                              uint32_t nowMs, uint32_t lastTriggerMs,
                              uint32_t cooldownMs);
bool xiaozhiShouldAutoTrigger(bool enabled, bool soundTriggered,
                              bool micSuppressed,
                              uint32_t nowMs, uint32_t lastTriggerMs,
                              uint32_t cooldownMs);
bool xiaozhiMicSuppressed(bool speakerPlaying,
                          uint32_t nowMs,
                          uint32_t mutedUntilMs);
void formatXiaozhiLocalReply(char *out, size_t outSize, float tempC,
                             float humidity, bool presence,
                             bool securityArmed);
