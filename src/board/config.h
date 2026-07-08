#pragma once

#include <Arduino.h>

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
constexpr int PIN_BUTTON_3 = 41;
constexpr int PIN_BUTTON_4 = 13;
constexpr int PIN_BUTTON_5 = 21;
constexpr int PIN_IR_TX = 14;
constexpr int PIN_MIC_SCK = 15;
constexpr int PIN_PRESENCE_OUT = 16;
constexpr int PIN_IR_RX = 17;
constexpr int PIN_STATUS_NEOPIXEL = 18;
constexpr int PIN_MIC_WS = 39;
constexpr int PIN_MIC_SD = 40;

constexpr int NEOPIXEL_COUNT = 30;
constexpr int IR_DEMO_BURST_CYCLES = 3000;
constexpr uint32_t IR_TEST_WINDOW_MS = 5000;
constexpr uint32_t IR_TEST_BURST_GAP_MS = 120;
constexpr int BUZZER_ACTIVE_LEVEL = HIGH;
constexpr int BUTTON_ACTIVE_LEVEL = HIGH;
constexpr int PRESENCE_ACTIVE_LEVEL = HIGH;
constexpr int IR_RX_ACTIVE_LEVEL = LOW;
constexpr uint32_t IR_RX_HOLD_MS = 1000;
constexpr int MIC_SAMPLE_RATE = 16000;
constexpr int MIC_SAMPLE_SHIFT = 8;
constexpr int MIC_RMS_TRIGGER = 65000;
constexpr int MIC_RMS_ADAPT_MARGIN = 35000;
constexpr int MIC_RMS_ADAPT_MULTIPLIER_PERCENT = 240;
constexpr bool MIC_DEBUG = true;

constexpr float TEMP_COOLING_THRESHOLD_C = 28.0;
constexpr uint32_t EMPTY_TO_SECURITY_MS = 10000;
constexpr uint32_t ALARM_HOLD_MS = 5000;
constexpr uint32_t AC_COMMAND_GAP_MS = 15000;
constexpr uint32_t PHONE_ALERT_COOLDOWN_MS = 60000;
