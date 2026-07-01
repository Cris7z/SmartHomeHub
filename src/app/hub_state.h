#pragma once

#include <Arduino.h>

struct HubState {
  bool ahtOk = false;
  bool i2sOk = false;
  bool presence = false;
  bool soundTriggered = false;
  int32_t micLevel = 0;
  bool irReceived = false;
  bool securityArmed = false;
  bool alarm = false;
  bool forceSecurity = false;
  bool manualLamp = false;
  bool manualAc = false;
  bool acCooling = false;
  float tempC = 26.5;
  float humidity = 55.0;
  uint32_t lastSeenMs = 0;
  uint32_t alarmUntilMs = 0;
  uint32_t lastAcCommandMs = 0;
  uint32_t buzzerTestUntilMs = 0;
};

extern HubState state;
