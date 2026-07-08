#include <cassert>

#include "../src/board/rgb_pins.h"

int main() {
  assert(PIN_STATUS_NEOPIXEL == 18);
  assert(PIN_ONBOARD_RGB == 48);
  assert(PIN_STATUS_NEOPIXEL != PIN_ONBOARD_RGB);
  assert(ONBOARD_RGB_COUNT == 1);
  assert(shouldClearOnboardRgb(PIN_ONBOARD_RGB, PIN_STATUS_NEOPIXEL));
  assert(!shouldClearOnboardRgb(PIN_STATUS_NEOPIXEL, PIN_STATUS_NEOPIXEL));
  return 0;
}
