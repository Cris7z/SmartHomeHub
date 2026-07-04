#include <cassert>

#include "../src/board/lamp_effect.h"

int main() {
  LampColor off = lampColorFor(false, false, 0);
  assert(off.r == 0 && off.g == 0 && off.b == 0);

  LampColor occupied = lampColorFor(true, false, 0);
  assert(occupied.r == 255 && occupied.g == 180 && occupied.b == 80);

  LampColor alarmOn = lampColorFor(false, true, 0);
  assert(alarmOn.r == 255 && alarmOn.g == 0 && alarmOn.b == 0);

  LampColor alarmOff = lampColorFor(true, true, 250);
  assert(alarmOff.r == 0 && alarmOff.g == 0 && alarmOff.b == 0);

  LampColor alarmOnAgain = lampColorFor(false, true, 500);
  assert(alarmOnAgain.r == 255 && alarmOnAgain.g == 0 && alarmOnAgain.b == 0);

  return 0;
}
