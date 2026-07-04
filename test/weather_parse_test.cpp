#include <cassert>
#include <cstring>

#include "../src/net/weather_parse.h"

int main() {
  IpLocationSnapshot location;
  const char *ipJson =
      "{\"status\":\"success\",\"city\":\"Xiamen\",\"lat\":24.4798,"
      "\"lon\":118.0894,\"timezone\":\"Asia/Shanghai\"}";
  assert(parseIpLocation(ipJson, location));
  assert(std::strcmp(location.city, "Xiamen") == 0);
  assert(std::strcmp(location.timezone, "Asia/Shanghai") == 0);
  assert(location.latitude > 24.47f && location.latitude < 24.49f);
  assert(location.longitude > 118.08f && location.longitude < 118.10f);

  WeatherSnapshot weather;
  const char *weatherJson =
      "{\"utc_offset_seconds\":28800,\"current_weather\":{\"temperature\":29.6,\"windspeed\":9.1,"
      "\"weathercode\":2},\"daily\":{\"sunrise\":[\"2026-07-04T05:24\"],"
      "\"sunset\":[\"2026-07-04T19:00\"]}}";
  assert(parseWeatherSnapshot(weatherJson, weather));
  assert(weather.temperatureC > 29.5f && weather.temperatureC < 29.7f);
  assert(weather.windKph > 9.0f && weather.windKph < 9.2f);
  assert(weather.weatherCode == 2);
  assert(weather.utcOffsetSeconds == 28800);
  assert(std::strcmp(weather.sunrise, "05:24") == 0);
  assert(std::strcmp(weather.sunset, "19:00") == 0);

  return 0;
}
