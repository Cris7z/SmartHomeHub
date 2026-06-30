/*
  Lexin Smart Home Hub - ESP32-S3 demo

  Board: ESP32-S3-DevKitC / ESP32S3 Dev Module
  Core libraries needed:
    - Adafruit GFX Library
    - Adafruit ILI9341
    - Adafruit AHTX0
    - Adafruit NeoPixel
*/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_NeoPixel.h>
#include <driver/i2s.h>

// ---------- Pins ----------
// I2C sensor
constexpr int PIN_I2C_SDA = 4;
constexpr int PIN_I2C_SCL = 5;

// ILI9341 SPI screen
constexpr int PIN_TFT_SCK = 12;
constexpr int PIN_TFT_MOSI = 11;
constexpr int PIN_TFT_CS = 10;
constexpr int PIN_TFT_DC = 9;
constexpr int PIN_TFT_RST = 8;
constexpr int PIN_TFT_BL = 7;

// Smart-home demo modules
constexpr int PIN_BUZZER = 6;
constexpr int PIN_BUTTON_1 = 1;
constexpr int PIN_BUTTON_2 = 2;
constexpr int PIN_BUTTON_3 = 3;
constexpr int PIN_BUTTON_4 = 13;
constexpr int PIN_BUTTON_5 = 21;
constexpr int PIN_IR_TX = 14;
constexpr int PIN_MIC_SCK = 15;
constexpr int PIN_PRESENCE_OUT = 16;
constexpr int PIN_IR_RX = 17;
constexpr int PIN_STATUS_NEOPIXEL = 18;
constexpr int PIN_MIC_WS = 19;
constexpr int PIN_MIC_SD = 20;

// ---------- Module settings ----------
constexpr int NEOPIXEL_COUNT = 30;
constexpr int BUZZER_ACTIVE_LEVEL = HIGH;   // High-level trigger active buzzer.
constexpr int BUTTON_ACTIVE_LEVEL = HIGH;   // VCC/OUT/GND button modules usually output HIGH when pressed.
constexpr int PRESENCE_ACTIVE_LEVEL = HIGH; // LD2410 OUT is usually high when presence is detected.
constexpr int IR_RX_ACTIVE_LEVEL = LOW;     // Common 38 kHz IR receiver modules idle HIGH and pulse LOW.
constexpr int MIC_SAMPLE_RATE = 16000;
constexpr int MIC_RMS_TRIGGER = 65000;       // Raise if it triggers too easily, lower if it is insensitive.

constexpr float TEMP_COOLING_THRESHOLD_C = 28.0;
constexpr uint32_t EMPTY_TO_SECURITY_MS = 10000;
constexpr uint32_t ALARM_HOLD_MS = 5000;
constexpr uint32_t AC_COMMAND_GAP_MS = 15000;

Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_MOSI, PIN_TFT_SCK, PIN_TFT_RST);
Adafruit_AHTX0 aht;
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, PIN_STATUS_NEOPIXEL, NEO_GRB + NEO_KHZ800);

struct HubState {
  bool ahtOk = false;
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

HubState state;

uint32_t lastSensorReadMs = 0;
uint32_t lastUiDrawMs = 0;
uint32_t lastMicReadMs = 0;

constexpr int BUTTON_COUNT = 5;
const int BUTTON_PINS[BUTTON_COUNT] = {
  PIN_BUTTON_1,
  PIN_BUTTON_2,
  PIN_BUTTON_3,
  PIN_BUTTON_4,
  PIN_BUTTON_5
};
int lastButtonLevel[BUTTON_COUNT] = {
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL,
  !BUTTON_ACTIVE_LEVEL
};
uint32_t lastButtonChangeMs[BUTTON_COUNT] = {0, 0, 0, 0, 0};

void buzzerWrite(bool on) {
  digitalWrite(PIN_BUZZER, on ? BUZZER_ACTIVE_LEVEL : !BUZZER_ACTIVE_LEVEL);
}

void setLamp(bool on, bool alarm) {
  uint32_t color = strip.Color(0, 0, 0);
  if (alarm) {
    color = strip.Color(255, 0, 0);
  } else if (on) {
    color = strip.Color(255, 180, 80);
  }

  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void sendIrDemoBurst() {
  // Demo-only 38 kHz carrier burst. It proves the IR LED is driven, but it is not
  // a real air-conditioner protocol. Replace with a learned AC library/code later.
  const int cycles = 80;
  for (int i = 0; i < cycles; i++) {
    digitalWrite(PIN_IR_TX, HIGH);
    delayMicroseconds(13);
    digitalWrite(PIN_IR_TX, LOW);
    delayMicroseconds(13);
  }
}

bool setupI2sMic() {
  i2s_config_t i2sConfig = {};
  i2sConfig.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  i2sConfig.sample_rate = MIC_SAMPLE_RATE;
  i2sConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  i2sConfig.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2sConfig.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2sConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2sConfig.dma_buf_count = 4;
  i2sConfig.dma_buf_len = 128;
  i2sConfig.use_apll = false;
  i2sConfig.tx_desc_auto_clear = false;
  i2sConfig.fixed_mclk = 0;

  i2s_pin_config_t pinConfig = {};
  pinConfig.bck_io_num = PIN_MIC_SCK;
  pinConfig.ws_io_num = PIN_MIC_WS;
  pinConfig.data_out_num = I2S_PIN_NO_CHANGE;
  pinConfig.data_in_num = PIN_MIC_SD;

  if (i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, nullptr) != ESP_OK) {
    return false;
  }
  if (i2s_set_pin(I2S_NUM_0, &pinConfig) != ESP_OK) {
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  return true;
}

void readMicrophone() {
  if (millis() - lastMicReadMs < 120) return;
  lastMicReadMs = millis();

  int32_t samples[128];
  size_t bytesRead = 0;
  esp_err_t result = i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesRead, 2);
  if (result != ESP_OK || bytesRead == 0) {
    state.micLevel = 0;
    state.soundTriggered = false;
    return;
  }

  const int count = bytesRead / sizeof(samples[0]);
  int64_t sumSquares = 0;
  for (int i = 0; i < count; i++) {
    int32_t sample = samples[i] >> 14;
    sumSquares += (int64_t)sample * sample;
  }

  state.micLevel = sqrt((double)sumSquares / count);
  state.soundTriggered = state.micLevel > MIC_RMS_TRIGGER;
}

void handleButtonPress(int index) {
  switch (index) {
    case 0:
      state.forceSecurity = !state.forceSecurity;
      Serial.printf("[KEY1] Force security: %s\n", state.forceSecurity ? "ON" : "OFF");
      break;
    case 1:
      state.manualLamp = !state.manualLamp;
      Serial.printf("[KEY2] Manual lamp: %s\n", state.manualLamp ? "ON" : "OFF");
      break;
    case 2:
      state.manualAc = !state.manualAc;
      Serial.printf("[KEY3] Manual AC: %s\n", state.manualAc ? "ON" : "OFF");
      break;
    case 3:
      state.buzzerTestUntilMs = millis() + 300;
      Serial.println("[KEY4] Buzzer test");
      break;
    case 4:
      state.alarm = false;
      state.forceSecurity = false;
      Serial.println("[KEY5] Alarm/security cleared");
      break;
  }
}

void readEnvironment() {
  if (millis() - lastSensorReadMs < 2000) return;
  lastSensorReadMs = millis();

  if (state.ahtOk) {
    sensors_event_t humidityEvent;
    sensors_event_t tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);
    state.tempC = tempEvent.temperature;
    state.humidity = humidityEvent.relative_humidity;
  } else {
    // Keep the interface alive when the sensor is not connected yet.
    state.tempC = 26.0 + 2.0 * sin(millis() / 9000.0);
    state.humidity = 55.0 + 8.0 * sin(millis() / 13000.0);
  }
}

void readInputs() {
  state.presence = digitalRead(PIN_PRESENCE_OUT) == PRESENCE_ACTIVE_LEVEL;
  state.irReceived = digitalRead(PIN_IR_RX) == IR_RX_ACTIVE_LEVEL;
  readMicrophone();

  if (state.presence) {
    state.lastSeenMs = millis();
  }

  for (int i = 0; i < BUTTON_COUNT; i++) {
    int buttonLevel = digitalRead(BUTTON_PINS[i]);
    if (buttonLevel != lastButtonLevel[i] && millis() - lastButtonChangeMs[i] > 50) {
      lastButtonChangeMs[i] = millis();
      lastButtonLevel[i] = buttonLevel;
      if (buttonLevel == BUTTON_ACTIVE_LEVEL) {
        handleButtonPress(i);
      }
    }
  }
}

void updateAutomation() {
  const bool roomEmptyLongEnough = millis() - state.lastSeenMs > EMPTY_TO_SECURITY_MS;
  state.securityArmed = state.forceSecurity || (!state.presence && roomEmptyLongEnough);

  if (state.securityArmed && state.soundTriggered) {
    state.alarm = true;
    state.alarmUntilMs = millis() + ALARM_HOLD_MS;
  }
  if (state.alarm && millis() > state.alarmUntilMs) {
    state.alarm = false;
  }

  state.acCooling = state.manualAc || state.tempC >= TEMP_COOLING_THRESHOLD_C;
  if (state.acCooling && millis() - state.lastAcCommandMs > AC_COMMAND_GAP_MS) {
    state.lastAcCommandMs = millis();
    sendIrDemoBurst();
    Serial.println("[AC] Demo IR cooling command sent");
  }

  buzzerWrite(state.alarm || millis() < state.buzzerTestUntilMs);
  setLamp(state.presence || state.manualLamp, state.alarm);
}

void drawStatusRow(int y, const char *label, const char *value, uint16_t color) {
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
  tft.setCursor(18, y);
  tft.print(label);

  tft.setTextSize(3);
  tft.setTextColor(color, ILI9341_BLACK);
  tft.setCursor(118, y - 5);
  tft.print(value);
}

void drawUi() {
  if (millis() - lastUiDrawMs < 500) return;
  lastUiDrawMs = millis();

  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 320, 34, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setCursor(12, 9);
  tft.print("Lexin Smart Home");

  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(4);
  tft.setCursor(18, 52);
  tft.printf("%.1fC", state.tempC);

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setCursor(180, 52);
  tft.printf("%.0f%%", state.humidity);

  drawStatusRow(115, "ROOM", state.presence ? "OCCUPIED" : "EMPTY",
                state.presence ? ILI9341_GREEN : ILI9341_ORANGE);
  drawStatusRow(150, "SEC", state.securityArmed ? "ARMED" : "OFF",
                state.securityArmed ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawStatusRow(185, "AC", state.acCooling ? "COOLING" : "STANDBY",
                state.acCooling ? ILI9341_CYAN : ILI9341_DARKGREY);
  drawStatusRow(220, "IR", state.irReceived ? "RX" : "IDLE",
                state.irReceived ? ILI9341_MAGENTA : ILI9341_DARKGREY);

  if (state.alarm) {
    tft.fillRect(0, 246, 320, 24, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(64, 250);
    tft.print("ALARM: NOISE!");
  } else if (state.forceSecurity) {
    tft.fillRect(0, 246, 320, 24, ILI9341_DARKGREEN);
    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(28, 250);
    tft.print("MODE: FORCED SECURITY");
  } else if (state.manualLamp || state.manualAc) {
    tft.fillRect(0, 246, 320, 24, ILI9341_DARKCYAN);
    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
    tft.setTextSize(2);
    tft.setCursor(32, 250);
    tft.print("MANUAL: ");
    if (state.manualLamp) tft.print("LAMP ");
    if (state.manualAc) tft.print("AC");
  }
}

void printSerialStatus() {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < 2000) return;
  lastPrintMs = millis();

  Serial.printf("T=%.1fC H=%.0f%% presence=%d security=%d sound=%d mic=%ld ir=%d alarm=%d ac=%d lamp=%d\n",
                state.tempC, state.humidity, state.presence, state.securityArmed,
                state.soundTriggered, (long)state.micLevel, state.irReceived, state.alarm, state.acCooling,
                state.manualLamp);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("Lexin Smart Home Hub booting...");

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_TFT_BL, OUTPUT);
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_IR_RX, INPUT_PULLUP);
  for (int i = 0; i < BUTTON_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT);
  }
  pinMode(PIN_PRESENCE_OUT, INPUT_PULLUP);
  buzzerWrite(false);
  digitalWrite(PIN_IR_TX, LOW);
  digitalWrite(PIN_TFT_BL, HIGH);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  state.ahtOk = aht.begin(&Wire);
  Serial.printf("AHT20: %s\n", state.ahtOk ? "OK" : "not found, using demo data");
  Serial.printf("I2S mic: %s\n", setupI2sMic() ? "OK" : "not found");

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(24, 100);
  tft.print("Smart Home Hub");

  strip.begin();
  strip.setBrightness(60);
  strip.show();

  state.lastSeenMs = millis();
}

void loop() {
  readEnvironment();
  readInputs();
  updateAutomation();
  drawUi();
  printSerialStatus();
}
