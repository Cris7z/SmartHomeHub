#include "time_weather.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

#include "../app/hub_state.h"
#include "secrets_config.h"
#include "weather_parse.h"

namespace {
constexpr uint32_t TIME_UPDATE_MS = 1000;
constexpr uint32_t WEATHER_UPDATE_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t WEATHER_RETRY_MS = 30UL * 1000UL;
constexpr uint32_t LOCATION_UPDATE_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr uint32_t LOCATION_RETRY_MS = 60UL * 1000UL;

uint32_t lastTimeUpdateMs = 0;
uint32_t lastWeatherAttemptMs = 0;
uint32_t lastLocationAttemptMs = 0;
bool fallbackLocationApplied = false;

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

void applyFallbackLocation() {
  if (fallbackLocationApplied || state.locationReady) return;
  fallbackLocationApplied = true;
  state.weatherLat = String(SMART_HOME_WEATHER_LAT).toFloat();
  state.weatherLon = String(SMART_HOME_WEATHER_LON).toFloat();
  strlcpy(state.locationText, "Manual", sizeof(state.locationText));
  strlcpy(state.timezoneText, "Asia/Shanghai", sizeof(state.timezoneText));
}

void applyUtcOffset(int utcOffsetSeconds) {
  if (state.utcOffsetSeconds == utcOffsetSeconds) return;
  state.utcOffsetSeconds = utcOffsetSeconds;
  configTime(state.utcOffsetSeconds, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
  state.timeReady = false;
  lastTimeUpdateMs = 0;
}

void fetchLocation() {
  if (WiFi.status() != WL_CONNECTED) return;

  const uint32_t now = millis();
  const uint32_t interval = state.locationReady ? LOCATION_UPDATE_MS : LOCATION_RETRY_MS;
  if (lastLocationAttemptMs != 0 && now - lastLocationAttemptMs < interval) return;
  lastLocationAttemptMs = now;

  HTTPClient http;
  http.setTimeout(3500);
  http.begin("http://ip-api.com/json/?fields=status,message,city,lat,lon,timezone");
  const int code = http.GET();
  if (code == 200) {
    const String body = http.getString();
    IpLocationSnapshot location;
    if (parseIpLocation(body.c_str(), location)) {
      state.locationReady = true;
      state.weatherReady = false;
      state.weatherLat = location.latitude;
      state.weatherLon = location.longitude;
      strlcpy(state.locationText, location.city[0] ? location.city : "Local", sizeof(state.locationText));
      strlcpy(state.timezoneText, location.timezone[0] ? location.timezone : "auto", sizeof(state.timezoneText));
      lastWeatherAttemptMs = 0;
      Serial.printf("[Location] %s %.4f,%.4f %s\n",
                    state.locationText, state.weatherLat, state.weatherLon, state.timezoneText);
    } else {
      applyFallbackLocation();
      Serial.println("[Location] parse failed, using fallback coordinates");
    }
  } else {
    applyFallbackLocation();
    Serial.printf("[Location] HTTP code: %d, using fallback coordinates\n", code);
  }
  http.end();
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  applyFallbackLocation();

  const uint32_t now = millis();
  const uint32_t interval = state.weatherReady ? WEATHER_UPDATE_MS : WEATHER_RETRY_MS;
  if (lastWeatherAttemptMs != 0 && now - lastWeatherAttemptMs < interval) return;
  lastWeatherAttemptMs = now;

  String url = "http://api.open-meteo.com/v1/forecast?latitude=";
  url += String(state.weatherLat, 4);
  url += "&longitude=";
  url += String(state.weatherLon, 4);
  url += "&current_weather=true&daily=sunrise,sunset&timezone=auto&forecast_days=1";

  HTTPClient http;
  http.setTimeout(5000);
  http.begin(url);
  const int code = http.GET();
  if (code == 200) {
    const String body = http.getString();
    WeatherSnapshot weather;
    if (parseWeatherSnapshot(body.c_str(), weather)) {
      state.weatherReady = true;
      state.outdoorTempC = weather.temperatureC;
      state.windKph = weather.windKph;
      state.weatherCode = weather.weatherCode;
      strlcpy(state.weatherText, weatherCodeName(weather.weatherCode), sizeof(state.weatherText));
      strlcpy(state.sunriseText, weather.sunrise, sizeof(state.sunriseText));
      strlcpy(state.sunsetText, weather.sunset, sizeof(state.sunsetText));
      applyUtcOffset(weather.utcOffsetSeconds);
      Serial.printf("[Weather] %s %.1fC %s wind=%.1fkm/h rise=%s set=%s code=%d\n",
                    state.locationText, state.outdoorTempC, state.weatherText, state.windKph,
                    state.sunriseText, state.sunsetText, state.weatherCode);
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
  applyFallbackLocation();
}

void updateTimeWeather() {
  updateClockText();
  fetchLocation();
  fetchWeather();
}
