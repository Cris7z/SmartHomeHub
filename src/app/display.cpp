#include "display.h"

#include <Arduino.h>

#include "display_cache.h"
#include "event_log.h"
#include "hub_state.h"
#include "../board/hardware.h"
#include "../io/mic_processing.h"

namespace {
uint32_t lastUiDrawMs = 0;
uint8_t lastDrawnPage = 255;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr uint32_t UI_REFRESH_MS = 100;
constexpr uint32_t HEARTBEAT_MS = 500;
constexpr int HEADER_HEIGHT = 30;
constexpr int BANNER_Y = 214;
constexpr int BANNER_HEIGHT = SCREEN_HEIGHT - BANNER_Y;
constexpr uint16_t COLOR_BG = ILI9341_BLACK;
constexpr uint16_t COLOR_HEADER = 0x020F;
constexpr uint16_t COLOR_ACCENT = 0x07FF;
constexpr uint16_t COLOR_PANEL = 0x18E3;
constexpr uint16_t COLOR_PANEL_2 = 0x2126;
constexpr uint16_t COLOR_MUTED = ILI9341_LIGHTGREY;

enum DynamicSlot : uint8_t {
  SlotCardLeft = 0,
  SlotCardRight,
  SlotRow1,
  SlotRow2,
  SlotRow3,
  SlotRow4,
  DynamicSlotCount
};

DisplayCacheSlot headerTimeCache;
DisplayCacheSlot headerBeatCache;
DisplayCacheSlot bannerCache;
DisplayCacheSlot homeCache[DynamicSlotCount];
DisplayCacheSlot weatherCache[DynamicSlotCount];
DisplayCacheSlot systemCache[DynamicSlotCount];
DisplayCacheSlot eventsCache[DynamicSlotCount];

bool updateTextColorCache(DisplayCacheSlot &slot, const char *value, uint16_t color) {
  char key[DISPLAY_CACHE_TEXT_LEN];
  snprintf(key, sizeof(key), "%04X:%s", (unsigned)color, value ? value : "");
  return updateDisplayCacheSlot(slot, key);
}

void resetDynamicCaches() {
  resetDisplayCacheSlot(headerTimeCache);
  resetDisplayCacheSlot(headerBeatCache);
  resetDisplayCacheSlot(bannerCache);
  resetDisplayCacheSlots(homeCache, DynamicSlotCount);
  resetDisplayCacheSlots(weatherCache, DynamicSlotCount);
  resetDisplayCacheSlots(systemCache, DynamicSlotCount);
  resetDisplayCacheSlots(eventsCache, DynamicSlotCount);
}

void copyShortText(char *out, size_t outSize, const char *text) {
  if (!out || outSize == 0) return;
  strlcpy(out, text ? text : "", outSize);
}

void drawHeaderStatic(const char *title) {
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER);
  tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_ACCENT);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, COLOR_HEADER);
  tft.setCursor(10, 7);
  tft.print(title);
}

void drawHeaderDynamic() {
  const char *timeText = state.timeReady ? state.timeText : "--:--";
  if (updateDisplayCacheSlot(headerTimeCache, timeText)) {
    tft.fillRect(224, 7, 62, 16, COLOR_HEADER);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE, COLOR_HEADER);
    tft.setCursor(224, 7);
    tft.print(timeText);
  }

  const bool beatOn = (millis() / HEARTBEAT_MS) % 2 == 0;
  if (updateDisplayCacheSlot(headerBeatCache, beatOn ? "1" : "0")) {
    tft.fillCircle(302, 15, 7, COLOR_HEADER);
    tft.fillCircle(302, 15, 5, beatOn ? ILI9341_CYAN : ILI9341_DARKGREY);
  }
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

void drawCachedCardValue(DisplayCacheSlot &slot, int x, int y, int w, int h,
                         const char *value, uint16_t valueColor) {
  if (!updateTextColorCache(slot, value, valueColor)) return;
  drawCardValue(x, y, w, h, value, valueColor);
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

void drawCachedRowValue(DisplayCacheSlot &slot, int y, const char *value, uint16_t color) {
  if (!updateTextColorCache(slot, value, color)) return;
  drawRowValue(y, value, color);
}

void drawPageDots() {
  const int startX = 132;
  for (int i = 0; i < 4; i++) {
    const uint16_t color = state.displayPage == i ? ILI9341_CYAN : ILI9341_DARKGREY;
    tft.fillCircle(startX + i * 18, 224, 4, color);
  }
}

void drawBannerDynamic() {
  char bannerKey[16];
  snprintf(bannerKey, sizeof(bannerKey), "%u:%u", state.alarm ? 1 : 0, state.displayPage);
  if (!updateDisplayCacheSlot(bannerCache, bannerKey)) return;

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

  drawCachedCardValue(homeCache[SlotCardLeft], 14, 42, 140, 54, tempText, ILI9341_CYAN);
  drawCachedCardValue(homeCache[SlotCardRight], 166, 42, 140, 54, humText, ILI9341_GREEN);
  drawCachedRowValue(homeCache[SlotRow1], 112, state.presence ? "OCCUPIED" : "EMPTY", state.presence ? ILI9341_GREEN : ILI9341_ORANGE);
  drawCachedRowValue(homeCache[SlotRow2], 140, state.securityArmed ? "ARMED" : "OFF", state.securityArmed ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawCachedRowValue(homeCache[SlotRow3], 168, state.acCooling ? "COOLING" : "STANDBY", state.acCooling ? ILI9341_CYAN : ILI9341_DARKGREY);
  drawCachedRowValue(homeCache[SlotRow4], 196, state.irReceived ? "RX" : "IDLE", state.irReceived ? ILI9341_MAGENTA : ILI9341_DARKGREY);
}

void drawWeatherStatic() {
  drawCommonStatic("WEATHER");
  drawCardFrame(14, 42, 140, 54, "OUTDOOR");
  drawCardFrame(166, 42, 140, 54, "CONDITION");
  drawRowLabel(112, "CITY");
  drawRowLabel(140, "RISE");
  drawRowLabel(168, "WIFI");
  drawRowLabel(196, "SET");
}

void drawWeatherDynamic() {
  char outText[16];
  char cityText[16];
  snprintf(outText, sizeof(outText), "%.1fC", state.outdoorTempC);
  strlcpy(cityText, state.locationText, sizeof(cityText));

  drawCachedCardValue(weatherCache[SlotCardLeft], 14, 42, 140, 54, state.weatherReady ? outText : "--", ILI9341_ORANGE);
  drawCachedCardValue(weatherCache[SlotCardRight], 166, 42, 140, 54, state.weatherReady ? state.weatherText : "WAIT", ILI9341_CYAN);
  drawCachedRowValue(weatherCache[SlotRow1], 112, state.locationReady ? cityText : "Manual", state.locationReady ? ILI9341_GREEN : ILI9341_ORANGE);
  drawCachedRowValue(weatherCache[SlotRow2], 140, state.weatherReady ? state.sunriseText : "--:--", ILI9341_LIGHTGREY);
  drawCachedRowValue(weatherCache[SlotRow3], 168, state.wifiStaConnected ? "CONNECTED" : "OFFLINE", state.wifiStaConnected ? ILI9341_GREEN : ILI9341_RED);
  drawCachedRowValue(weatherCache[SlotRow4], 196, state.weatherReady ? state.sunsetText : "--:--", ILI9341_LIGHTGREY);
}

void drawSystemStatic() {
  drawCommonStatic("SYSTEM");
  drawCardFrame(14, 42, 140, 54, "NOISE");
  drawCardFrame(166, 42, 140, 54, "BLE");
  drawRowLabel(112, "SOUND");
  drawRowLabel(140, "LIMIT");
  drawRowLabel(168, "LAMP");
  drawRowLabel(196, "IR TEST");
}

void drawSystemDynamic() {
  char micText[20];
  char thresholdText[20];
  snprintf(micText, sizeof(micText), "%d%%", noisePercentFor(state.micLevel, state.micThreshold));
  snprintf(thresholdText, sizeof(thresholdText), "%s", state.adaptiveMicReady ? "100%" : "CAL...");

  drawCachedCardValue(systemCache[SlotCardLeft], 14, 42, 140, 54, micText, state.soundTriggered ? ILI9341_YELLOW : ILI9341_CYAN);
  drawCachedCardValue(systemCache[SlotCardRight], 166, 42, 140, 54, state.bleClientConnected ? "CONNECTED" : "WAIT", state.bleClientConnected ? ILI9341_GREEN : ILI9341_DARKGREY);
  drawCachedRowValue(systemCache[SlotRow1], 112, state.soundTriggered ? "LOUD" : "QUIET", state.soundTriggered ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawCachedRowValue(systemCache[SlotRow2], 140, thresholdText, state.adaptiveMicReady ? ILI9341_GREEN : ILI9341_DARKGREY);
  drawCachedRowValue(systemCache[SlotRow3], 168, state.lampOverride ? (state.manualLamp ? "MANUAL ON" : "MANUAL OFF") : "AUTO", state.lampOverride ? ILI9341_CYAN : ILI9341_GREEN);
  drawCachedRowValue(systemCache[SlotRow4], 196, state.irTestActive ? "RUNNING" : "IDLE", state.irTestActive ? ILI9341_MAGENTA : ILI9341_DARKGREY);
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
  drawCachedCardValue(eventsCache[SlotCardLeft], 14, 42, 140, 54, riskText, riskColor);
  drawCachedCardValue(eventsCache[SlotCardRight], 166, 42, 140, 54, thresholdText, ILI9341_CYAN);
  for (uint8_t i = 0; i < HUB_EVENT_LOG_COUNT; i++) {
    char lineText[16];
    const char *line = eventLogLine(i);
    copyShortText(lineText, sizeof(lineText), line[0] ? line : "--");
    drawCachedRowValue(eventsCache[SlotRow1 + i], 112 + i * 28, lineText, ILI9341_WHITE);
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
    resetDynamicCaches();
    lastDrawnPage = state.displayPage;
    drawStaticForPage(state.displayPage);
  }

  drawHeaderDynamic();
  drawDynamicForPage(state.displayPage);
  drawBannerDynamic();
}
