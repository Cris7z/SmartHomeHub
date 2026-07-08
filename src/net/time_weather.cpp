#include "time_weather.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

#include "../app/hub_state.h"
#include "secrets_config.h"

namespace {
constexpr uint32_t TIME_UPDATE_MS = 1000;
constexpr uint32_t WEATHER_UPDATE_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t WEATHER_RETRY_MS = 30UL * 1000UL;

uint32_t lastTimeUpdateMs = 0;
uint32_t lastWeatherAttemptMs = 0;

const char *weatherCodeName(int code) {
  if (code == 0) return "Clear";
  if (code <= 3) return "Cloudy";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 67) return "Drizzle";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Rain";
  if (code >= 95) return "Thunder";
  return "Weather";
}

bool extractFloatFrom(const String &body, const char *key, float &value, int startIndex) {
  const int keyIndex = body.indexOf(key, startIndex);
  if (keyIndex < 0) return false;
  const int colonIndex = body.indexOf(':', keyIndex);
  if (colonIndex < 0) return false;
  value = body.substring(colonIndex + 1).toFloat();
  return true;
}

bool extractIntFrom(const String &body, const char *key, int &value, int startIndex) {
  float parsed = 0.0;
  if (!extractFloatFrom(body, key, parsed, startIndex)) return false;
  value = (int)parsed;
  return true;
}

void updateClockText() {
  if (millis() - lastTimeUpdateMs < TIME_UPDATE_MS) return;
  lastTimeUpdateMs = millis();

  struct tm timeInfo = {};
  if (!getLocalTime(&timeInfo, 50)) {
    state.timeReady = false;
    return;
  }

  state.timeReady = true;
  strftime(state.timeText, sizeof(state.timeText), "%H:%M", &timeInfo);
  strftime(state.dateText, sizeof(state.dateText), "%m-%d", &timeInfo);
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;

  const uint32_t now = millis();
  const uint32_t interval = state.weatherReady ? WEATHER_UPDATE_MS : WEATHER_RETRY_MS;
  if (now - lastWeatherAttemptMs < interval) return;
  lastWeatherAttemptMs = now;

  String url = "http://api.open-meteo.com/v1/forecast?latitude=";
  url += SMART_HOME_WEATHER_LAT;
  url += "&longitude=";
  url += SMART_HOME_WEATHER_LON;
  url += "&current_weather=true";

  HTTPClient http;
  http.begin(url);
  const int code = http.GET();
  if (code == 200) {
    const String body = http.getString();
    float temperature = 0.0;
    float windspeed = 0.0;
    int weatherCode = -1;
    const int weatherStart = body.indexOf("\"current_weather\"");
    const bool ok = weatherStart >= 0 &&
                    extractFloatFrom(body, "\"temperature\"", temperature, weatherStart) &&
                    extractFloatFrom(body, "\"windspeed\"", windspeed, weatherStart) &&
                    extractIntFrom(body, "\"weathercode\"", weatherCode, weatherStart);
    if (ok) {
      state.weatherReady = true;
      state.outdoorTempC = temperature;
      state.windKph = windspeed;
      state.weatherCode = weatherCode;
      strlcpy(state.weatherText, weatherCodeName(weatherCode), sizeof(state.weatherText));
      Serial.printf("[Weather] %.1fC %s wind=%.1fkm/h code=%d\n",
                    state.outdoorTempC, state.weatherText, state.windKph, state.weatherCode);
    } else {
      state.weatherReady = false;
      strlcpy(state.weatherText, "ParseErr", sizeof(state.weatherText));
      Serial.println("[Weather] parse failed");
    }
  } else {
    state.weatherReady = false;
    strlcpy(state.weatherText, "HttpErr", sizeof(state.weatherText));
    Serial.printf("[Weather] HTTP code: %d\n", code);
  }
  http.end();
}
}

void setupTimeWeather() {
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
}

void updateTimeWeather() {
  updateClockText();
  fetchWeather();
}
