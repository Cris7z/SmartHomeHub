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
constexpr int MIC_RMS_TRIGGER = 36000;
constexpr int MIC_RMS_ADAPT_MARGIN = 9000;
constexpr int MIC_RMS_ADAPT_MULTIPLIER_PERCENT = 120;
constexpr int MIC_SMOOTH_WEIGHT_PERCENT = 35;
constexpr int MIC_ALARM_RMS_TRIGGER = 70000;
constexpr bool MIC_DEBUG = true;
constexpr uint32_t VOICE_INPUT_SAMPLE_RATE = 16000;
constexpr uint32_t VOICE_OUTPUT_SAMPLE_RATE = 24000;
constexpr size_t VOICE_OUTPUT_BUFFER_SECONDS = 8;
constexpr uint16_t VOICE_PACKET_MS = 20;
constexpr size_t VOICE_PACKET_SAMPLES = 320;
constexpr size_t VOICE_PACKET_BYTES = 640;
constexpr uint16_t VOICE_PREROLL_MS = 300;
constexpr size_t VOICE_PREROLL_SAMPLES = 4800;
constexpr uint8_t VOICE_PCM_RIGHT_SHIFT = 16;
constexpr uint32_t VOICE_TURN_MIN_MS = 1800;
constexpr uint32_t VOICE_TURN_SILENCE_MS = 900;
constexpr uint32_t VOICE_TURN_MAX_MS = 6000;
constexpr int SPEAKER_SAMPLE_RATE = 16000;
constexpr int SPEAKER_TONE_HZ = 880;
constexpr int SPEAKER_TONE_MS = 220;
constexpr int SPEAKER_TONE_AMPLITUDE = 1800;
constexpr size_t SPEAKER_TONE_CHUNK_FRAMES = 256;
constexpr size_t STREAMING_SPEAKER_WRITE_BUDGET_FRAMES = SPEAKER_TONE_CHUNK_FRAMES * 4;
constexpr uint32_t STREAMING_SPEAKER_DRAIN_MS = 140;
constexpr int VOICE_OUTPUT_GAIN_PERCENT = 240;
constexpr bool XIAOZHI_AUTO_WAKE_ENABLED = false;
constexpr uint32_t XIAOZHI_AUTO_WAKE_COOLDOWN_MS = 8000;
constexpr uint32_t XIAOZHI_AUTO_REARM_QUIET_MS = 1200;
constexpr uint32_t XIAOZHI_LISTEN_MS = 800;
constexpr uint32_t XIAOZHI_THINK_MS = 600;
constexpr uint32_t XIAOZHI_SPEAK_MS = 900;
constexpr uint32_t XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS = 6000;
constexpr uint32_t XIAOZHI_MANUAL_RESTART_MS = 1200;

constexpr float TEMP_COOLING_THRESHOLD_C = 28.0;
constexpr float TEMP_STANDBY_THRESHOLD_C = 25.0;
constexpr uint32_t EMPTY_TO_SECURITY_MS = 10000;
constexpr uint32_t ALARM_ARM_GRACE_MS = 1800;
constexpr uint32_t ALARM_SOUND_CONFIRM_MS = 800;
constexpr uint32_t ALARM_HOLD_MS = 5000;
constexpr uint32_t AC_COMMAND_GAP_MS = 15000;
constexpr uint32_t PHONE_ALERT_COOLDOWN_MS = 60000;
