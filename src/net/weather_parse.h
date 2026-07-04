#pragma once

struct IpLocationSnapshot {
  char city[32] = "";
  char timezone[40] = "";
  float latitude = 0.0f;
  float longitude = 0.0f;
};

struct WeatherSnapshot {
  float temperatureC = 0.0f;
  float windKph = 0.0f;
  int weatherCode = -1;
  int utcOffsetSeconds = 8 * 3600;
  char sunrise[8] = "--:--";
  char sunset[8] = "--:--";
};

bool parseIpLocation(const char *body, IpLocationSnapshot &snapshot);
bool parseWeatherSnapshot(const char *body, WeatherSnapshot &snapshot);
