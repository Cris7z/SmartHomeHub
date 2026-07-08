/*
  Lexin Smart Home Hub - ESP32-S3 demo

  Board: ESP32-S3-DevKitC / ESP32S3 Dev Module
  Framework: Arduino on PlatformIO
*/

#include <Arduino.h>
#include <Wire.h>

#include "app/automation.h"
#include "app/ai_guard.h"
#include "app/diagnostics.h"
#include "app/display.h"
#include "app/hub_state.h"
#include "app/xiaozhi_ai.h"
#include "board/config.h"
#include "board/hardware.h"
#include "io/inputs.h"
#include "io/sensors.h"
#include "net/ble_service.h"
#include "net/pushplus.h"
#include "net/time_weather.h"
#include "net/voice_stream_client.h"
#include "net/web_dashboard.h"

void setup() {
  clearOnboardRgb();

  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("Lexin Smart Home Hub booting...");

  setupOutputPins();
  setupInputs();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  state.ahtOk = aht.begin(&Wire);
  Serial.printf("AHT20: %s\n", state.ahtOk ? "OK" : "not found, using demo data");
  state.i2sOk = setupI2sMic();
  Serial.printf("I2S mic: %s\n", state.i2sOk ? "OK" : "not found");
  state.i2sSpeakerOk = setupI2sSpeaker();
  Serial.printf("I2S speaker: %s\n", state.i2sSpeakerOk ? "OK" : "not found");
  setupVoiceStreamClient();
  setupXiaozhiAi();

  setupDisplay();
  setupStatusLamp();
  setupPhoneAlert();
  setupTimeWeather();
  setupWebDashboard();
  setupBleService();

  state.lastSeenMs = millis();
}

void loop() {
  updateVoiceStreamClient();
  updateStreamingSpeaker();
  updateSpeakerAudio();
  readEnvironment();
  readInputs();
  updateAutomation();
  updateAiGuard();
  updateXiaozhiAi();
  updatePhoneAlert();
  updateTimeWeather();
  updateWebDashboard();
  updateBleService();
  drawUi();
  printSerialStatus();
}
