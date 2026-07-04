#pragma once

#include <cstdint>

struct AlarmStateUpdate {
  bool alarm = false;
  bool justTriggered = false;
  bool justCleared = false;
  uint32_t alarmUntilMs = 0;
};

AlarmStateUpdate updateAlarmState(bool securityArmed,
                                  bool soundTriggered,
                                  bool currentAlarm,
                                  uint32_t currentAlarmUntilMs,
                                  uint32_t nowMs,
                                  uint32_t quietHoldMs);
