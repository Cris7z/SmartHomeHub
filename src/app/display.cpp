#include "display.h"

#include <Arduino.h>

#include "event_log.h"
#include "hub_state.h"
#include "../board/hardware.h"

namespace {
uint32_t lastUiDrawMs = 0;
uint8_t lastDrawnPage = 255;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr uint32_t UI_REFRESH_MS = 250;
constexpr uint32_t HEARTBEAT_MS = 500;
constexpr int HEADER_HEIGHT = 30;
constexpr int BANNER_Y = 214;
constexpr int BANNER_HEIGHT = SCREEN_HEIGHT - BANNER_Y;
constexpr uint16_t COLOR_BG = ILI9341_BLACK;
constexpr uint16_t COLOR_PANEL = 0x18E3;
constexpr uint16_t COLOR_PANEL_2 = 0x2126;
constexpr uint16_t COLOR_MUTED = ILI9341_LIGHTGREY;

void drawHeaderStatic(const char *title) {
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_NAVY);
  tft.setCursor(10, 7);
  tft.print(title);
}

void drawHeaderDynamic() {
  tft.fillRect(224, 7, 62, 16, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_NAVY);
  tft.setCursor(224, 7);
  tft.print(state.timeReady ? state.timeText : "--:--");

  tft.fillCircle(302, 15, 7, ILI9341_NAVY);
  tft.fillCircle(302, 15, 6, (millis() / HEARTBEAT_MS) % 2 == 0 ? ILI9341_CYAN : ILI9341_DARKGREY);
}

void drawCardFrame(int x, int y, int w, int h, const char *label) {
  tft.fillRoundRect(x, y, w, h, 5, COLOR_PANEL);
  tft.drawRoundRect(x, y, w, h, 5, COLOR_PANEL_2);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
  tft.setCursor(x + 8, y + 7);
  tft.print(label);
}

void drawCardValue(int x, int y, int w, int h, const char *value, uint16_t valueColor) {
  tft.fillRect(x + 7, y + 23, w - 14, h - 28, COLOR_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(valueColor, COLOR_PANEL);
  tft.setCursor(x + 8, y + 25);
  tft.print(value);
}

void drawRowLabel(int y, const char *label) {
  tft.setTextSize(2);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.setCursor(18, y);
  tft.print(label);
}

void drawRowValue(int y, const char *value, uint16_t color) {
  tft.fillRect(128, y - 2, 182, 20, COLOR_BG);
  tft.setTextSize(2);
  tft.setTextColor(color, COLOR_BG);
  tft.setCursor(128, y);
  tft.print(value);
}

void drawPageDots() {
  const int startX = 132;
  for (int i = 0; i < 4; i++) {
    const uint16_t color = state.displayPage == i ? ILI9341_CYAN : ILI9341_DARKGREY;
    tft.fillCircle(startX + i * 18, 224, 4, color);
  }
}

void drawBannerDynamic() {
  tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, COLOR_BG);
  if (state.alarm) {
    tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(82, BANNER_Y + 5);
    tft.print("ALARM: NOISE!");
  } else {
    drawPageDots();
  }
}

void drawCommonStatic(const char *title) {
  tft.fillScreen(COLOR_BG);
  drawHeaderStatic(title);
}

void drawHomeStatic() {
  drawCommonStatic("HOME");
  drawCardFrame(14, 42, 140, 54, "INDOOR TEMP");
  drawCardFrame(166, 42, 140, 54, "HUMIDITY");
  drawRowLabel(112, "ROOM");
  drawRowLabel(140, "SEC");
  drawRowLabel(168, "AC");
  drawRowLabel(196, "IR");
}

void drawHomeDynamic() {
  char tempText[16];
  char humText[16];
  snprintf(tempText, sizeof(tempText), "%.1fC", state.tempC);
  snprintf(humText, sizeof(humText), "%.0f%%", state.humidity);

  drawCardValue(14, 42, 140, 54, tempText, ILI9341_CYAN);
  drawCardValue(166, 42, 140, 54, humText, ILI9341_GREEN);
  drawRowValue(112, state.presence ? "OCCUPIED" : "EMPTY", state.presence ? ILI9341_GREEN : ILI9341_ORANGE);
  drawRowValue(140, state.securityArmed ? "ARMED" : "OFF", state.securityArmed ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawRowValue(168, state.acCooling ? "COOLING" : "STANDBY", state.acCooling ? ILI9341_CYAN : ILI9341_DARKGREY);
  drawRowValue(196, state.irReceived ? "RX" : "IDLE", state.irReceived ? ILI9341_MAGENTA : ILI9341_DARKGREY);
}

void drawWeatherStatic() {
  drawCommonStatic("WEATHER");
  drawCardFrame(14, 42, 140, 54, "OUTDOOR");
  drawCardFrame(166, 42, 140, 54, "CONDITION");
  drawRowLabel(112, "DATE");
  drawRowLabel(140, "WIND");
  drawRowLabel(168, "WIFI");
  drawRowLabel(196, "PUSH");
}

void drawWeatherDynamic() {
  char outText[16];
  char windText[20];
  snprintf(outText, sizeof(outText), "%.1fC", state.outdoorTempC);
  snprintf(windText, sizeof(windText), "%.1f km/h", state.windKph);

  drawCardValue(14, 42, 140, 54, state.weatherReady ? outText : "--", ILI9341_ORANGE);
  drawCardValue(166, 42, 140, 54, state.weatherReady ? state.weatherText : "WAIT", ILI9341_CYAN);
  drawRowValue(112, state.timeReady ? state.dateText : "--", ILI9341_WHITE);
  drawRowValue(140, state.weatherReady ? windText : "--", ILI9341_LIGHTGREY);
  drawRowValue(168, state.wifiStaConnected ? "CONNECTED" : "OFFLINE", state.wifiStaConnected ? ILI9341_GREEN : ILI9341_RED);
  drawRowValue(196, state.pushPlusConfigured ? "READY" : "NO TOKEN", state.pushPlusConfigured ? ILI9341_GREEN : ILI9341_ORANGE);
}

void drawSystemStatic() {
  drawCommonStatic("SYSTEM");
  drawCardFrame(14, 42, 140, 54, "MIC RMS");
  drawCardFrame(166, 42, 140, 54, "BLE");
  drawRowLabel(112, "SOUND");
  drawRowLabel(140, "THRESH");
  drawRowLabel(168, "LAMP");
  drawRowLabel(196, "IR TEST");
}

void drawSystemDynamic() {
  char micText[20];
  char thresholdText[20];
  snprintf(micText, sizeof(micText), "%ld", (long)state.micLevel);
  snprintf(thresholdText, sizeof(thresholdText), "%ld", (long)state.micThreshold);

  drawCardValue(14, 42, 140, 54, micText, state.soundTriggered ? ILI9341_YELLOW : ILI9341_CYAN);
  drawCardValue(166, 42, 140, 54, state.bleClientConnected ? "CONNECTED" : "WAIT", state.bleClientConnected ? ILI9341_GREEN : ILI9341_DARKGREY);
  drawRowValue(112, state.soundTriggered ? "TRIGGER" : "IDLE", state.soundTriggered ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawRowValue(140, thresholdText, state.adaptiveMicReady ? ILI9341_GREEN : ILI9341_DARKGREY);
  drawRowValue(168, state.lampOverride ? (state.manualLamp ? "MANUAL ON" : "MANUAL OFF") : "AUTO", state.lampOverride ? ILI9341_CYAN : ILI9341_GREEN);
  drawRowValue(196, state.irTestActive ? "RUNNING" : "IDLE", state.irTestActive ? ILI9341_MAGENTA : ILI9341_DARKGREY);
}

void drawEventsStatic() {
  drawCommonStatic("AI GUARD");
  drawCardFrame(14, 42, 140, 54, "RISK SCORE");
  drawCardFrame(166, 42, 140, 54, "AI THRESHOLD");
  drawRowLabel(112, "LOG1");
  drawRowLabel(140, "LOG2");
  drawRowLabel(168, "LOG3");
  drawRowLabel(196, "LOG4");
}

void drawEventsDynamic() {
  char riskText[20];
  char thresholdText[20];
  snprintf(riskText, sizeof(riskText), "%u %s", state.aiRiskScore, state.aiRiskText);
  snprintf(thresholdText, sizeof(thresholdText), "%ld", (long)state.micThreshold);

  const uint16_t riskColor = state.aiRiskScore >= 80 ? ILI9341_RED :
                             state.aiRiskScore >= 55 ? ILI9341_ORANGE :
                             state.aiRiskScore >= 30 ? ILI9341_YELLOW : ILI9341_GREEN;
  drawCardValue(14, 42, 140, 54, riskText, riskColor);
  drawCardValue(166, 42, 140, 54, thresholdText, ILI9341_CYAN);
  for (uint8_t i = 0; i < HUB_EVENT_LOG_COUNT; i++) {
    drawRowValue(112 + i * 28, eventLogLine(i)[0] ? eventLogLine(i) : "--", ILI9341_WHITE);
  }
}

void drawStaticForPage(uint8_t page) {
  switch (page) {
    case 1:
      drawWeatherStatic();
      break;
    case 2:
      drawSystemStatic();
      break;
    case 3:
      drawEventsStatic();
      break;
    default:
      drawHomeStatic();
      break;
  }
}

void drawDynamicForPage(uint8_t page) {
  switch (page) {
    case 1:
      drawWeatherDynamic();
      break;
    case 2:
      drawSystemDynamic();
      break;
    case 3:
      drawEventsDynamic();
      break;
    default:
      drawHomeDynamic();
      break;
  }
}
}

void setupDisplay() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);
}

void drawUi() {
  if (millis() - lastUiDrawMs < UI_REFRESH_MS) return;
  lastUiDrawMs = millis();

  if (lastDrawnPage != state.displayPage) {
    lastDrawnPage = state.displayPage;
    drawStaticForPage(state.displayPage);
  }

  drawHeaderDynamic();
  drawDynamicForPage(state.displayPage);
  drawBannerDynamic();
}
