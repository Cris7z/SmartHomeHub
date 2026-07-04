#include "weather_parse.h"

#include <cstdlib>
#include <cstring>

namespace {
bool copyJsonString(const char *body, const char *key, char *out, size_t outSize, const char *scope = nullptr) {
  if (!body || !key || !out || outSize == 0) return false;
  const char *start = scope ? std::strstr(body, scope) : body;
  if (!start) return false;

  const char *keyPos = std::strstr(start, key);
  if (!keyPos) return false;
  const char *colon = std::strchr(keyPos, ':');
  if (!colon) return false;
  const char *firstQuote = std::strchr(colon, '"');
  if (!firstQuote) return false;
  const char *secondQuote = std::strchr(firstQuote + 1, '"');
  if (!secondQuote) return false;

  const size_t len = (size_t)(secondQuote - firstQuote - 1);
  const size_t copyLen = len < outSize - 1 ? len : outSize - 1;
  std::memcpy(out, firstQuote + 1, copyLen);
  out[copyLen] = '\0';
  return true;
}

bool extractFloat(const char *body, const char *key, float &value, const char *scope = nullptr) {
  if (!body || !key) return false;
  const char *start = scope ? std::strstr(body, scope) : body;
  if (!start) return false;

  const char *keyPos = std::strstr(start, key);
  if (!keyPos) return false;
  const char *colon = std::strchr(keyPos, ':');
  if (!colon) return false;

  char *end = nullptr;
  value = std::strtof(colon + 1, &end);
  return end && end != colon + 1;
}

bool extractInt(const char *body, const char *key, int &value, const char *scope = nullptr) {
  float parsed = 0.0f;
  if (!extractFloat(body, key, parsed, scope)) return false;
  value = (int)parsed;
  return true;
}

void copyClockFromIso(const char *isoText, char *out, size_t outSize) {
  if (!isoText || !out || outSize == 0) return;
  const char *timeStart = std::strchr(isoText, 'T');
  if (!timeStart) {
    out[0] = '\0';
    return;
  }
  timeStart++;
  const size_t copyLen = outSize > 6 ? 5 : outSize - 1;
  std::memcpy(out, timeStart, copyLen);
  out[copyLen] = '\0';
}
}

bool parseIpLocation(const char *body, IpLocationSnapshot &snapshot) {
  char status[12] = "";
  if (!copyJsonString(body, "\"status\"", status, sizeof(status))) return false;
  if (std::strcmp(status, "success") != 0) return false;

  return copyJsonString(body, "\"city\"", snapshot.city, sizeof(snapshot.city)) &&
         copyJsonString(body, "\"timezone\"", snapshot.timezone, sizeof(snapshot.timezone)) &&
         extractFloat(body, "\"lat\"", snapshot.latitude) &&
         extractFloat(body, "\"lon\"", snapshot.longitude);
}

bool parseWeatherSnapshot(const char *body, WeatherSnapshot &snapshot) {
  char sunriseIso[24] = "";
  char sunsetIso[24] = "";
  const bool ok = extractFloat(body, "\"temperature\"", snapshot.temperatureC, "\"current_weather\"") &&
                  extractFloat(body, "\"windspeed\"", snapshot.windKph, "\"current_weather\"") &&
                  extractInt(body, "\"weathercode\"", snapshot.weatherCode, "\"current_weather\"") &&
                  copyJsonString(body, "\"sunrise\"", sunriseIso, sizeof(sunriseIso), "\"daily\"") &&
                  copyJsonString(body, "\"sunset\"", sunsetIso, sizeof(sunsetIso), "\"daily\"");
  if (!ok) return false;

  int parsedOffset = snapshot.utcOffsetSeconds;
  if (extractInt(body, "\"utc_offset_seconds\"", parsedOffset)) {
    snapshot.utcOffsetSeconds = parsedOffset;
  }
  copyClockFromIso(sunriseIso, snapshot.sunrise, sizeof(snapshot.sunrise));
  copyClockFromIso(sunsetIso, snapshot.sunset, sizeof(snapshot.sunset));
  return snapshot.sunrise[0] != '\0' && snapshot.sunset[0] != '\0';
}
