#pragma once

#include <cstddef>
#include <cstdint>

enum class WeatherIconKind {
  Unknown = 0,
  Sun,
  Cloud,
  Rain,
  Storm,
  Fog,
  Snow
};

enum class WeatherMetricKind {
  Location = 0,
  Timezone,
  Bluetooth,
  Wind,
  Sunrise,
  Sunset,
  Wifi,
  Date
};

struct WeatherDisplaySnapshot {
  bool weatherReady = false;
  bool locationReady = false;
  bool wifiConnected = false;
  bool bleConnected = false;
  int32_t utcOffsetSeconds = 8 * 3600;
  float outdoorTempC = 0.0f;
  float windKph = 0.0f;
  const char *locationText = nullptr;
  const char *weatherText = nullptr;
  const char *sunriseText = nullptr;
  const char *sunsetText = nullptr;
  const char *dateText = nullptr;
};

WeatherIconKind weatherIconKindForCode(int weatherCode);
void formatBlinkingTime(char *out, std::size_t outSize, const char *timeText, bool colonVisible);
void formatWeatherMetricText(char *out, std::size_t outSize, WeatherMetricKind kind,
                             const WeatherDisplaySnapshot &snapshot);
