#include "lamp_effect.h"

LampColor lampColorFor(bool on, bool alarm, uint32_t nowMs) {
  if (alarm) {
    const bool blinkOn = ((nowMs / LAMP_ALARM_BLINK_MS) % 2U) == 0U;
    return blinkOn ? LampColor(255, 0, 0) : LampColor(0, 0, 0);
  }

  if (on) {
    return LampColor(255, 180, 80);
  }

  return LampColor(0, 0, 0);
}
