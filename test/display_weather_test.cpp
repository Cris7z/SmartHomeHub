#include <cassert>
#include <cstring>

#include "../src/app/display_weather.h"

int main() {
  assert(weatherIconKindForCode(0) == WeatherIconKind::Sun);
  assert(weatherIconKindForCode(2) == WeatherIconKind::Cloud);
  assert(weatherIconKindForCode(45) == WeatherIconKind::Fog);
  assert(weatherIconKindForCode(61) == WeatherIconKind::Rain);
  assert(weatherIconKindForCode(73) == WeatherIconKind::Snow);
  assert(weatherIconKindForCode(81) == WeatherIconKind::Rain);
  assert(weatherIconKindForCode(95) == WeatherIconKind::Storm);
  assert(weatherIconKindForCode(-1) == WeatherIconKind::Unknown);

  char out[8];
  formatBlinkingTime(out, sizeof(out), "12:34", true);
  assert(std::strcmp(out, "12:34") == 0);
  formatBlinkingTime(out, sizeof(out), "12:34", false);
  assert(std::strcmp(out, "12 34") == 0);
  formatBlinkingTime(out, sizeof(out), "--:--", false);
  assert(std::strcmp(out, "-- --") == 0);

  WeatherDisplaySnapshot snapshot;
  snapshot.weatherReady = true;
  snapshot.locationReady = true;
  snapshot.wifiConnected = true;
  snapshot.bleConnected = true;
  snapshot.utcOffsetSeconds = 8 * 3600;
  snapshot.outdoorTempC = 28.5f;
  snapshot.windKph = 8.5f;
  snapshot.locationText = "Xiamen";
  snapshot.weatherText = "Clear";
  snapshot.sunriseText = "05:23";
  snapshot.sunsetText = "19:00";
  snapshot.dateText = "07-04";

  char metric[16];
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Location, snapshot);
  assert(std::strcmp(metric, "Xiamen") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Timezone, snapshot);
  assert(std::strcmp(metric, "UTC+8") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Bluetooth, snapshot);
  assert(std::strcmp(metric, "LINK") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Wind, snapshot);
  assert(std::strcmp(metric, "8.5km/h") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Sunrise, snapshot);
  assert(std::strcmp(metric, "05:23") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Sunset, snapshot);
  assert(std::strcmp(metric, "19:00") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Wifi, snapshot);
  assert(std::strcmp(metric, "ON") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Date, snapshot);
  assert(std::strcmp(metric, "07-04") == 0);

  snapshot.weatherReady = false;
  snapshot.locationReady = false;
  snapshot.wifiConnected = false;
  snapshot.bleConnected = false;
  snapshot.utcOffsetSeconds = 0;
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Timezone, snapshot);
  assert(std::strcmp(metric, "UTC+0") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Bluetooth, snapshot);
  assert(std::strcmp(metric, "WAIT") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Location, snapshot);
  assert(std::strcmp(metric, "Manual") == 0);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Wifi, snapshot);
  assert(std::strcmp(metric, "OFF") == 0);

  return 0;
}
