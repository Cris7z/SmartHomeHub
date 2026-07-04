#include "alarm_logic.h"

namespace {
bool timeReached(uint32_t nowMs, uint32_t deadlineMs) {
  return (int32_t)(nowMs - deadlineMs) >= 0;
}
}

AlarmStateUpdate updateAlarmState(bool securityArmed,
                                  bool soundTriggered,
                                  bool currentAlarm,
                                  uint32_t currentAlarmUntilMs,
                                  uint32_t nowMs,
                                  uint32_t quietHoldMs) {
  AlarmStateUpdate result;
  result.alarm = currentAlarm;
  result.alarmUntilMs = currentAlarmUntilMs;

  const bool noisyWhileArmed = securityArmed && soundTriggered;
  if (noisyWhileArmed) {
    result.justTriggered = !currentAlarm;
    result.alarm = true;
    result.alarmUntilMs = nowMs + quietHoldMs;
    return result;
  }

  if (currentAlarm && timeReached(nowMs, currentAlarmUntilMs)) {
    result.alarm = false;
    result.alarmUntilMs = 0;
    result.justCleared = true;
  }

  return result;
}
