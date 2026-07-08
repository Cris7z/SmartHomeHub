#pragma once

constexpr int PIN_STATUS_NEOPIXEL = 18;
constexpr int NEOPIXEL_COUNT = 30;

constexpr int PIN_ONBOARD_RGB = 48;
constexpr int ONBOARD_RGB_COUNT = 1;
constexpr bool CLEAR_ONBOARD_RGB_AT_BOOT = true;

constexpr bool shouldClearOnboardRgb(int onboardPin, int statusPin) {
  return CLEAR_ONBOARD_RGB_AT_BOOT && onboardPin >= 0 && onboardPin != statusPin;
}
