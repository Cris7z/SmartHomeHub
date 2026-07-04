#pragma once

#include <Arduino.h>

constexpr uint8_t HUB_EVENT_LOG_COUNT = 4;
constexpr size_t HUB_EVENT_TEXT_LEN = 40;

struct HubState {
  bool ahtOk = false;
  bool i2sOk = false;
  bool presence = false;
  bool soundTriggered = false;
  int32_t micLevel = 0;
  int32_t micBaseline = 0;
  int32_t micThreshold = 0;
  bool adaptiveMicReady = false;
  bool irReceived = false;
  bool securityArmed = false;
  bool alarm = false;
  bool forceSecurity = false;
  bool manualLamp = false;
  bool lampOverride = false;
  bool manualAc = false;
  bool acCooling = false;
  bool acCommandRequested = false;
  bool irTestActive = false;
  bool bleClientConnected = false;
  bool wifiStaConnected = false;
  bool pushPlusConfigured = false;
  bool timeReady = false;
  bool weatherReady = false;
  bool locationReady = false;
  uint8_t displayPage = 0;
  uint8_t aiRiskScore = 0;
  float tempC = 26.5;
  float humidity = 55.0;
  float outdoorTempC = 0.0;
  float windKph = 0.0;
  float weatherLat = 0.0;
  float weatherLon = 0.0;
  int weatherCode = -1;
  int32_t utcOffsetSeconds = 8 * 3600;
  char timeText[16] = "--:--";
  char dateText[16] = "----";
  char weatherText[20] = "Unknown";
  char locationText[32] = "Manual";
  char timezoneText[40] = "Asia/Shanghai";
  char sunriseText[8] = "--:--";
  char sunsetText[8] = "--:--";
  char aiRiskText[12] = "LOW";
  char ipText[16] = "--";
  char eventLog[HUB_EVENT_LOG_COUNT][HUB_EVENT_TEXT_LEN] = {};
  uint8_t eventLogHead = 0;
  uint8_t eventLogCount = 0;
  uint32_t lastSeenMs = 0;
  uint32_t alarmUntilMs = 0;
  uint32_t lastAcCommandMs = 0;
  uint32_t irReceivedUntilMs = 0;
  uint32_t irTestUntilMs = 0;
  uint32_t lastIrTestBurstMs = 0;
  uint32_t buzzerTestUntilMs = 0;
  uint32_t lastPhoneAlertMs = 0;
};

extern HubState state;
