#include "pushplus.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "../app/hub_state.h"
#include "secrets_config.h"

namespace {
bool hasValue(const char *value) {
  return value && value[0] != '\0';
}

const char *wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID";
    case WL_SCAN_COMPLETED:
      return "SCAN_DONE";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

uint32_t lastWifiRetryMs = 0;
uint32_t lastWifiLogMs = 0;
}

void setupPhoneAlert() {
  state.pushPlusConfigured = hasValue(SMART_HOME_PUSHPLUS_TOKEN);

  if (!hasValue(SMART_HOME_WIFI_SSID)) {
    Serial.println("Wi-Fi STA skipped: src/net/secrets.h is not configured");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(SMART_HOME_WIFI_SSID, SMART_HOME_WIFI_PASS);
  lastWifiRetryMs = millis();
  Serial.printf("Wi-Fi STA connecting to: %s\n", SMART_HOME_WIFI_SSID);
}

void updatePhoneAlert() {
  const wl_status_t status = WiFi.status();
  state.wifiStaConnected = status == WL_CONNECTED;
  state.pushPlusConfigured = hasValue(SMART_HOME_PUSHPLUS_TOKEN);
  if (state.wifiStaConnected) {
    strlcpy(state.ipText, WiFi.localIP().toString().c_str(), sizeof(state.ipText));
  } else {
    strlcpy(state.ipText, "--", sizeof(state.ipText));
  }

  if (hasValue(SMART_HOME_WIFI_SSID) && !state.wifiStaConnected &&
      millis() - lastWifiRetryMs > 10000) {
    lastWifiRetryMs = millis();
    WiFi.disconnect();
    WiFi.begin(SMART_HOME_WIFI_SSID, SMART_HOME_WIFI_PASS);
    Serial.println("[WiFi] reconnecting...");
  }

  if (millis() - lastWifiLogMs > 5000) {
    lastWifiLogMs = millis();
    Serial.printf("[WiFi] status=%s(%d) ip=%s\n",
                  wifiStatusName(status), (int)status, WiFi.localIP().toString().c_str());
  }
}

bool sendPhoneAlert() {
  updatePhoneAlert();
  if (!state.pushPlusConfigured) {
    Serial.println("[PushPlus] skipped: token is not configured");
    return false;
  }

  if (!state.wifiStaConnected) {
    Serial.println("[PushPlus] skipped: Wi-Fi STA is not connected");
    return false;
  }

  HTTPClient http;
  http.begin("http://www.pushplus.plus/send");
  http.addHeader("Content-Type", "application/json");

  String content = "Security mode detected abnormal noise.<br>";
  content += "Temp: " + String(state.tempC, 1) + "C<br>";
  content += "Humidity: " + String(state.humidity, 0) + "%<br>";
  content += "Mic RMS: " + String((long)state.micLevel);

  String body = "{";
  body += "\"token\":\"" + String(SMART_HOME_PUSHPLUS_TOKEN) + "\",";
  body += "\"title\":\"Smart Home Alert\",";
  body += "\"content\":\"" + content + "\",";
  body += "\"template\":\"html\"";
  body += "}";

  const int code = http.POST(body);
  Serial.printf("[PushPlus] HTTP code: %d\n", code);
  http.end();
  return code > 0 && code < 400;
}
