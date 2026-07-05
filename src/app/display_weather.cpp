#include "display_weather.h"

#include <cstdio>
#include <cstring>

namespace {
bool hasText(const char *text) {
  return text && text[0] != '\0';
}

void copyMetricText(char *out, std::size_t outSize, const char *text) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  if (!text) return;
  std::strncpy(out, text, outSize - 1);
  out[outSize - 1] = '\0';
}
}

WeatherIconKind weatherIconKindForCode(int weatherCode) {
  if (weatherCode == 0) return WeatherIconKind::Sun;
  if (weatherCode >= 1 && weatherCode <= 3) return WeatherIconKind::Cloud;
  if (weatherCode == 45 || weatherCode == 48) return WeatherIconKind::Fog;
  if (weatherCode >= 51 && weatherCode <= 67) return WeatherIconKind::Rain;
  if (weatherCode >= 71 && weatherCode <= 77) return WeatherIconKind::Snow;
  if (weatherCode >= 80 && weatherCode <= 82) return WeatherIconKind::Rain;
  if (weatherCode >= 95) return WeatherIconKind::Storm;
  return WeatherIconKind::Unknown;
}

void formatBlinkingTime(char *out, std::size_t outSize, const char *timeText, bool colonVisible) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  if (!timeText) return;

  std::strncpy(out, timeText, outSize - 1);
  out[outSize - 1] = '\0';
  if (!colonVisible && outSize > 3 && std::strlen(out) >= 3 && out[2] == ':') {
    out[2] = ' ';
  }
}

void formatWeatherMetricText(char *out, std::size_t outSize, WeatherMetricKind kind,
                             const WeatherDisplaySnapshot &snapshot) {
  if (!out || outSize == 0) return;
  out[0] = '\0';

  switch (kind) {
    case WeatherMetricKind::Location:
      copyMetricText(out, outSize,
                     snapshot.locationReady && hasText(snapshot.locationText)
                         ? snapshot.locationText
                         : "Manual");
      break;
    case WeatherMetricKind::Timezone: {
      const int32_t offset = snapshot.utcOffsetSeconds;
      const char sign = offset < 0 ? '-' : '+';
      const int32_t absSeconds = offset < 0 ? -offset : offset;
      const int32_t totalMinutes = absSeconds / 60;
      const int32_t hours = totalMinutes / 60;
      const int32_t minutes = totalMinutes % 60;
      if (minutes == 0) {
        std::snprintf(out, outSize, "UTC%c%ld", sign, (long)hours);
      } else {
        std::snprintf(out, outSize, "UTC%c%ld:%02ld", sign, (long)hours, (long)minutes);
      }
      break;
    }
    case WeatherMetricKind::Bluetooth:
      copyMetricText(out, outSize, snapshot.bleConnected ? "LINK" : "WAIT");
      break;
    case WeatherMetricKind::Wind:
      if (snapshot.weatherReady) {
        std::snprintf(out, outSize, "%.1fkm/h", snapshot.windKph);
      } else {
        copyMetricText(out, outSize, "--");
      }
      break;
    case WeatherMetricKind::Sunrise:
      copyMetricText(out, outSize,
                     snapshot.weatherReady && hasText(snapshot.sunriseText)
                         ? snapshot.sunriseText
                         : "--:--");
      break;
    case WeatherMetricKind::Sunset:
      copyMetricText(out, outSize,
                     snapshot.weatherReady && hasText(snapshot.sunsetText)
                         ? snapshot.sunsetText
                         : "--:--");
      break;
    case WeatherMetricKind::Wifi:
      copyMetricText(out, outSize, snapshot.wifiConnected ? "ON" : "OFF");
      break;
    case WeatherMetricKind::Date:
      copyMetricText(out, outSize, hasText(snapshot.dateText) ? snapshot.dateText : "----");
      break;
  }
}
