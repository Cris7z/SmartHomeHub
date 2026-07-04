#pragma once

#include <cstdint>

constexpr uint32_t LAMP_ALARM_BLINK_MS = 250;

struct LampColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  constexpr LampColor(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
      : r(red), g(green), b(blue) {}
};

LampColor lampColorFor(bool on, bool alarm, uint32_t nowMs);
