#pragma once

#include <Arduino.h>

#include "rgb_pins.h"

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
constexpr int PIN_MIC_WS = 39;
constexpr int PIN_MIC_SD = 40;
constexpr int PIN_SPEAKER_LRC = 38;
constexpr int PIN_SPEAKER_BCLK = 42;
constexpr int PIN_SPEAKER_DIN = 47;

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
constexpr int MIC_SMOOTH_WEIGHT_PERCENT = 20;
constexpr bool MIC_DEBUG = true;
constexpr int SPEAKER_SAMPLE_RATE = 16000;
constexpr int SPEAKER_TONE_HZ = 880;
constexpr int SPEAKER_TONE_MS = 220;
constexpr int SPEAKER_TONE_AMPLITUDE = 1800;
constexpr size_t SPEAKER_TONE_CHUNK_FRAMES = 256;
constexpr bool XIAOZHI_AUTO_WAKE_ENABLED = true;
constexpr uint32_t XIAOZHI_AUTO_WAKE_COOLDOWN_MS = 8000;
constexpr uint32_t XIAOZHI_LISTEN_MS = 800;
constexpr uint32_t XIAOZHI_THINK_MS = 600;
constexpr uint32_t XIAOZHI_SPEAK_MS = 900;
constexpr uint32_t XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS = 2500;

constexpr float TEMP_COOLING_THRESHOLD_C = 28.0;
constexpr uint32_t EMPTY_TO_SECURITY_MS = 10000;
constexpr uint32_t ALARM_HOLD_MS = 5000;
constexpr uint32_t AC_COMMAND_GAP_MS = 15000;
constexpr uint32_t PHONE_ALERT_COOLDOWN_MS = 60000;
