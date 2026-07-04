#include <cassert>

#include "../src/app/alarm_logic.h"

int main() {
  AlarmStateUpdate update = updateAlarmState(false, true, false, 0, 1000, 5000);
  assert(!update.alarm);
  assert(!update.justTriggered);

  update = updateAlarmState(true, true, false, 0, 1000, 5000);
  assert(update.alarm);
  assert(update.justTriggered);
  assert(!update.justCleared);
  assert(update.alarmUntilMs == 6000);

  update = updateAlarmState(true, false, true, 6000, 5999, 5000);
  assert(update.alarm);
  assert(!update.justCleared);

  update = updateAlarmState(true, false, true, 6000, 6000, 5000);
  assert(!update.alarm);
  assert(update.justCleared);
  assert(update.alarmUntilMs == 0);

  update = updateAlarmState(true, true, true, 6000, 4000, 5000);
  assert(update.alarm);
  assert(!update.justTriggered);
  assert(update.alarmUntilMs == 9000);

  update = updateAlarmState(true, false, true, 9000, 8999, 5000);
  assert(update.alarm);
  update = updateAlarmState(true, false, true, 9000, 9000, 5000);
  assert(!update.alarm);
  assert(update.justCleared);

  return 0;
}
