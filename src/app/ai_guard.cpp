#include "ai_guard.h"

#include <Arduino.h>

#include "hub_state.h"

namespace {
uint8_t clampScore(int score) {
  if (score < 0) return 0;
  if (score > 100) return 100;
  return (uint8_t)score;
}

uint8_t soundRiskScore() {
  if (state.soundTriggered) return 45;
  if (state.micThreshold <= 0) return 0;

  const int scaled = (int)((int64_t)state.micLevel * 35 / state.micThreshold);
  return clampScore(scaled > 35 ? 35 : scaled);
}
}

void updateAiGuard() {
  int score = 0;

  if (state.alarm) score += 100;
  if (state.securityArmed) score += 25;
  if (!state.presence) score += 15;
  if (state.irReceived) score += 10;
  if (state.tempC >= 30.0f) score += 10;
  score += soundRiskScore();

  state.aiRiskScore = clampScore(score);
  if (state.alarm || state.aiRiskScore >= 80) {
    strlcpy(state.aiRiskText, "ALARM", sizeof(state.aiRiskText));
  } else if (state.aiRiskScore >= 55) {
    strlcpy(state.aiRiskText, "HIGH", sizeof(state.aiRiskText));
  } else if (state.aiRiskScore >= 30) {
    strlcpy(state.aiRiskText, "MEDIUM", sizeof(state.aiRiskText));
  } else {
    strlcpy(state.aiRiskText, "LOW", sizeof(state.aiRiskText));
  }
}

