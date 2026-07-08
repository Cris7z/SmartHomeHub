#include "xiaozhi_cloud.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <cstring>

#include "secrets_config.h"
#include "xiaozhi_cloud_core.h"

namespace {
String cloudUrl() {
  String endpoint = SMART_HOME_XIAOZHI_ENDPOINT;
  const char *path = xiaozhiCloudPathForProvider(SMART_HOME_XIAOZHI_PROVIDER);
  if (endpoint.endsWith(path)) {
    return endpoint;
  }
  while (endpoint.endsWith("/")) {
    endpoint.remove(endpoint.length() - 1);
  }
  return endpoint + path;
}
}

bool isXiaozhiCloudConfigured() {
  return xiaozhiCloudConfigReady(SMART_HOME_XIAOZHI_PROVIDER,
                                 SMART_HOME_XIAOZHI_ENDPOINT,
                                 SMART_HOME_XIAOZHI_TOKEN,
                                 SMART_HOME_XIAOZHI_MODEL);
}

bool requestXiaozhiCloudReply(char *reply, size_t replySize,
                              const char *prompt,
                              float tempC, float humidity,
                              bool presence, bool securityArmed,
                              const char *weatherText, const char *timeText) {
  if (!reply || replySize == 0 || !isXiaozhiCloudConfigured()) {
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[XIAOZHI] cloud skipped: Wi-Fi is not connected");
    return false;
  }

  char homeStatus[160] = {};
  char payload[768] = {};
  if (!formatXiaozhiHomeStatus(homeStatus, sizeof(homeStatus),
                               tempC, humidity, presence, securityArmed,
                               weatherText, timeText)) {
    Serial.println("[XIAOZHI] cloud skipped: request payload too large");
    return false;
  }

  const XiaozhiCloudProvider provider = parseXiaozhiCloudProvider(SMART_HOME_XIAOZHI_PROVIDER);
  bool payloadOk = false;
  if (provider == XiaozhiCloudProvider::Anthropic) {
    payloadOk = buildXiaozhiAnthropicRequestJson(payload, sizeof(payload),
                                                SMART_HOME_XIAOZHI_MODEL,
                                                prompt, homeStatus);
  } else if (provider == XiaozhiCloudProvider::OpenAiChat) {
    payloadOk = buildXiaozhiOpenAiChatRequestJson(payload, sizeof(payload),
                                                 SMART_HOME_XIAOZHI_MODEL,
                                                 prompt, homeStatus);
  }
  if (!payloadOk) {
    Serial.println("[XIAOZHI] cloud skipped: request payload too large");
    return false;
  }

  HTTPClient http;
  http.setTimeout(8000);
  http.useHTTP10(true);
  http.begin(cloudUrl());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", String("Bearer ") + SMART_HOME_XIAOZHI_TOKEN);
  if (provider == XiaozhiCloudProvider::Anthropic) {
    http.addHeader("x-api-key", SMART_HOME_XIAOZHI_TOKEN);
    http.addHeader("anthropic-version", "2023-06-01");
  }

  const int code = http.POST((uint8_t *)payload, std::strlen(payload));
  bool ok = false;
  if (code == 200) {
    const String body = http.getString();
    ok = provider == XiaozhiCloudProvider::Anthropic
             ? parseXiaozhiAnthropicReply(body.c_str(), reply, replySize)
             : parseXiaozhiOpenAiChatReply(body.c_str(), reply, replySize);
    Serial.printf("[XIAOZHI] cloud HTTP 200 parse=%s\n", ok ? "OK" : "FAIL");
    if (!ok) {
      String head = body.substring(0, 220);
      head.replace("\n", " ");
      head.replace("\r", " ");
      Serial.printf("[XIAOZHI] cloud body len=%u head=%s\n",
                    (unsigned)body.length(), head.c_str());
    }
  } else {
    Serial.printf("[XIAOZHI] cloud HTTP code=%d\n", code);
  }
  http.end();
  return ok;
}
