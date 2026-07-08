#include "display.h"

#include <Arduino.h>
#include <math.h>

#include "display_cache.h"
#include "display_text.h"
#include "display_weather.h"
#include "event_log.h"
#include "hub_state.h"
#include "xiaozhi_core.h"
#include "../board/hardware.h"
#include "../io/mic_processing.h"

namespace {
uint32_t lastUiDrawMs = 0;
uint8_t lastDrawnPage = 255;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr uint32_t UI_REFRESH_MS = 100;
constexpr int HEADER_HEIGHT = 32;
constexpr int BANNER_Y = 216;
constexpr int BANNER_HEIGHT = SCREEN_HEIGHT - BANNER_Y;
constexpr int CARD_Y = 42;
constexpr int CARD_W = 144;
constexpr int CARD_H = 62;
constexpr int CARD_LEFT_X = 12;
constexpr int CARD_RIGHT_X = 164;
constexpr int ROW_VALUE_X = 116;
constexpr int ROW_VALUE_W = 192;
constexpr int ROW_H = 22;
constexpr int ROW_1_Y = 110;
constexpr int ROW_STEP = 26;
constexpr int WEATHER_TILE_Y = 112;
constexpr int WEATHER_TILE_W = 144;
constexpr int WEATHER_TILE_H = 24;
constexpr int WEATHER_TILE_STEP_Y = 25;
constexpr uint16_t COLOR_BG = 0x0841;
constexpr uint16_t COLOR_HEADER = 0x1082;
constexpr uint16_t COLOR_ACCENT = 0x07FF;
constexpr uint16_t COLOR_PANEL = 0x18C3;
constexpr uint16_t COLOR_PANEL_2 = 0x39E7;
constexpr uint16_t COLOR_CHIP = 0x2124;
constexpr uint16_t COLOR_MUTED = 0x9CF3;
constexpr uint16_t COLOR_SHADOW = ILI9341_BLACK;
constexpr uint8_t DISPLAY_PAGE_COUNT = 5;

enum DynamicSlot : uint8_t {
  SlotCardLeft = 0,
  SlotCardRight,
  SlotRow1,
  SlotRow2,
  SlotRow3,
  SlotRow4,
  SlotWeatherTile5,
  SlotWeatherTile6,
  SlotWeatherTile7,
  SlotWeatherTile8,
  DynamicSlotCount
};

DisplayCacheSlot headerTimeCache;
DisplayCacheSlot bannerCache;
DisplayCacheSlot homeCache[DynamicSlotCount];
DisplayCacheSlot weatherCache[DynamicSlotCount];
DisplayCacheSlot systemCache[DynamicSlotCount];
DisplayCacheSlot eventsCache[DynamicSlotCount];
DisplayCacheSlot xiaozhiCache[DynamicSlotCount];

bool updateTextColorCache(DisplayCacheSlot &slot, const char *value, uint16_t color) {
  char key[DISPLAY_CACHE_TEXT_LEN];
  snprintf(key, sizeof(key), "%04X:%s", (unsigned)color, value ? value : "");
  return updateDisplayCacheSlot(slot, key);
}

void resetDynamicCaches() {
  resetDisplayCacheSlot(headerTimeCache);
  resetDisplayCacheSlot(bannerCache);
  resetDisplayCacheSlots(homeCache, DynamicSlotCount);
  resetDisplayCacheSlots(weatherCache, DynamicSlotCount);
  resetDisplayCacheSlots(systemCache, DynamicSlotCount);
  resetDisplayCacheSlots(eventsCache, DynamicSlotCount);
  resetDisplayCacheSlots(xiaozhiCache, DynamicSlotCount);
}

void copyShortText(char *out, size_t outSize, const char *text) {
  fitDisplayText(out, outSize, text, 14);
}

void drawHeaderStatic(const char *title) {
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER);
  tft.fillRect(0, 0, 5, HEADER_HEIGHT, COLOR_ACCENT);
  tft.drawFastHLine(0, HEADER_HEIGHT - 1, SCREEN_WIDTH, COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_HEADER);
  tft.setCursor(13, 3);
  tft.print("LEXIN V1.1");
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, COLOR_HEADER);
  tft.setCursor(13, 15);
  tft.print(title);
}

void drawHeaderDynamic() {
  const char *timeText = state.timeReady ? state.timeText : "--:--";
  const bool colonVisible = (millis() / 1000) % 2 == 0;
  char renderedTime[8];
  formatBlinkingTime(renderedTime, sizeof(renderedTime), timeText, colonVisible);
  if (updateDisplayCacheSlot(headerTimeCache, renderedTime)) {
    tft.fillRoundRect(222, 5, 70, 22, 5, COLOR_CHIP);
    tft.drawRoundRect(222, 5, 70, 22, 5, COLOR_PANEL_2);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE, COLOR_CHIP);
    tft.setCursor(228, 8);
    tft.print(renderedTime);
  }
}

void drawCardFrame(int x, int y, int w, int h, const char *label) {
  tft.fillRoundRect(x + 2, y + 3, w, h, 6, COLOR_SHADOW);
  tft.fillRoundRect(x, y, w, h, 5, COLOR_PANEL);
  tft.drawRoundRect(x, y, w, h, 5, COLOR_PANEL_2);
  tft.fillRect(x + 8, y + h - 5, w - 16, 2, COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_PANEL);
  tft.setCursor(x + 8, y + 7);
  tft.print(label);
}

void drawCardValue(int x, int y, int w, int h, const char *value, uint16_t valueColor) {
  char fitted[20];
  const size_t valueLen = strlen(value ? value : "");
  const uint8_t textSize = valueLen <= 5 ? 3 : 2;
  const size_t maxChars = (w - 18) / (6 * textSize);
  fitDisplayText(fitted, sizeof(fitted), value, maxChars);

  tft.fillRect(x + 7, y + 22, w - 14, h - 30, COLOR_PANEL);
  tft.setTextSize(textSize);
  tft.setTextColor(valueColor, COLOR_PANEL);
  tft.setCursor(x + 9, y + (textSize == 3 ? 29 : 32));
  tft.print(fitted);
}

void drawCachedCardValue(DisplayCacheSlot &slot, int x, int y, int w, int h,
                         const char *value, uint16_t valueColor) {
  if (!updateTextColorCache(slot, value, valueColor)) return;
  drawCardValue(x, y, w, h, value, valueColor);
}

void drawCloudShape(int cx, int cy, uint16_t color) {
  tft.fillCircle(cx - 10, cy + 2, 7, color);
  tft.fillCircle(cx - 1, cy - 3, 9, color);
  tft.fillCircle(cx + 9, cy + 2, 8, color);
  tft.fillRoundRect(cx - 18, cy + 4, 38, 10, 4, color);
}

void drawWeatherIcon(WeatherIconKind kind, int cx, int cy) {
  switch (kind) {
    case WeatherIconKind::Sun:
      tft.fillCircle(cx, cy, 11, ILI9341_YELLOW);
      tft.drawCircle(cx, cy, 13, ILI9341_ORANGE);
      for (int i = 0; i < 8; i++) {
        const float angle = i * 0.785398f;
        const int x1 = cx + (int)(cos(angle) * 16);
        const int y1 = cy + (int)(sin(angle) * 16);
        const int x2 = cx + (int)(cos(angle) * 20);
        const int y2 = cy + (int)(sin(angle) * 20);
        tft.drawLine(x1, y1, x2, y2, ILI9341_YELLOW);
      }
      break;
    case WeatherIconKind::Cloud:
      drawCloudShape(cx, cy, ILI9341_LIGHTGREY);
      break;
    case WeatherIconKind::Rain:
      drawCloudShape(cx, cy - 5, ILI9341_LIGHTGREY);
      for (int i = -11; i <= 11; i += 11) {
        tft.drawLine(cx + i, cy + 12, cx + i - 4, cy + 20, ILI9341_CYAN);
      }
      break;
    case WeatherIconKind::Storm:
      drawCloudShape(cx, cy - 6, ILI9341_LIGHTGREY);
      tft.fillTriangle(cx - 2, cy + 8, cx + 8, cy + 8, cx + 1, cy + 19, ILI9341_YELLOW);
      tft.fillTriangle(cx + 1, cy + 16, cx + 10, cy + 16, cx - 2, cy + 27, ILI9341_YELLOW);
      break;
    case WeatherIconKind::Fog:
      for (int i = 0; i < 4; i++) {
        const int y = cy - 11 + i * 7;
        tft.drawFastHLine(cx - 22, y, 42, ILI9341_LIGHTGREY);
        tft.drawFastHLine(cx - 14, y + 3, 34, COLOR_MUTED);
      }
      break;
    case WeatherIconKind::Snow:
      drawCloudShape(cx, cy - 6, ILI9341_LIGHTGREY);
      for (int i = -11; i <= 11; i += 11) {
        const int sx = cx + i;
        const int sy = cy + 17;
        tft.drawLine(sx - 4, sy, sx + 4, sy, ILI9341_CYAN);
        tft.drawLine(sx, sy - 4, sx, sy + 4, ILI9341_CYAN);
      }
      break;
    default:
      tft.drawCircle(cx, cy, 12, COLOR_MUTED);
      tft.drawLine(cx - 7, cy, cx + 7, cy, COLOR_MUTED);
      break;
  }
}

void drawWeatherIconCard(int x, int y, int w, int h, WeatherIconKind kind, const char *label) {
  char fitted[12];
  fitDisplayText(fitted, sizeof(fitted), label, 9);

  tft.fillRect(x + 7, y + 20, w - 14, h - 27, COLOR_PANEL);
  drawWeatherIcon(kind, x + 43, y + 40);
  tft.setTextSize(1);
  tft.setTextColor(kind == WeatherIconKind::Unknown ? COLOR_MUTED : ILI9341_WHITE, COLOR_PANEL);
  tft.setCursor(x + 84, y + 37);
  tft.print(fitted);
}

void drawCachedWeatherIconCard(DisplayCacheSlot &slot, int weatherCode, const char *label) {
  const WeatherIconKind kind = weatherIconKindForCode(weatherCode);
  char key[DISPLAY_CACHE_TEXT_LEN];
  snprintf(key, sizeof(key), "%d:%s", (int)kind, label ? label : "");
  if (!updateDisplayCacheSlot(slot, key)) return;
  drawWeatherIconCard(CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, kind, label);
}

void drawTinyWeatherIcon(WeatherIconKind kind, int cx, int cy, uint16_t color) {
  switch (kind) {
    case WeatherIconKind::Sun:
      tft.fillCircle(cx, cy, 4, ILI9341_YELLOW);
      tft.drawFastHLine(cx - 8, cy, 4, ILI9341_YELLOW);
      tft.drawFastHLine(cx + 5, cy, 4, ILI9341_YELLOW);
      tft.drawFastVLine(cx, cy - 8, 4, ILI9341_YELLOW);
      tft.drawFastVLine(cx, cy + 5, 4, ILI9341_YELLOW);
      break;
    case WeatherIconKind::Cloud:
      tft.fillCircle(cx - 4, cy + 1, 3, color);
      tft.fillCircle(cx + 1, cy - 2, 5, color);
      tft.fillCircle(cx + 6, cy + 1, 4, color);
      tft.fillRoundRect(cx - 8, cy + 2, 16, 5, 2, color);
      break;
    case WeatherIconKind::Rain:
      drawTinyWeatherIcon(WeatherIconKind::Cloud, cx, cy - 2, ILI9341_LIGHTGREY);
      tft.drawLine(cx - 5, cy + 7, cx - 8, cy + 11, ILI9341_CYAN);
      tft.drawLine(cx + 4, cy + 7, cx + 1, cy + 11, ILI9341_CYAN);
      break;
    case WeatherIconKind::Storm:
      drawTinyWeatherIcon(WeatherIconKind::Cloud, cx, cy - 3, ILI9341_LIGHTGREY);
      tft.fillTriangle(cx - 1, cy + 4, cx + 5, cy + 4, cx, cy + 12, ILI9341_YELLOW);
      break;
    case WeatherIconKind::Fog:
      tft.drawFastHLine(cx - 9, cy - 4, 18, color);
      tft.drawFastHLine(cx - 6, cy + 1, 18, color);
      tft.drawFastHLine(cx - 9, cy + 6, 18, color);
      break;
    case WeatherIconKind::Snow:
      drawTinyWeatherIcon(WeatherIconKind::Cloud, cx, cy - 3, ILI9341_LIGHTGREY);
      tft.drawFastHLine(cx - 4, cy + 9, 8, ILI9341_CYAN);
      tft.drawFastVLine(cx, cy + 5, 8, ILI9341_CYAN);
      break;
    default:
      tft.drawCircle(cx, cy, 7, color);
      tft.drawFastHLine(cx - 4, cy, 8, color);
      break;
  }
}

void drawWifiArc(int cx, int cy, int radius, uint16_t color) {
  int lastX = cx;
  int lastY = cy;
  bool haveLast = false;
  for (int deg = 215; deg <= 325; deg += 10) {
    const float angle = deg * 0.0174533f;
    const int x = cx + (int)(cos(angle) * radius);
    const int y = cy + (int)(sin(angle) * radius);
    if (haveLast) {
      tft.drawLine(lastX, lastY, x, y, color);
    }
    lastX = x;
    lastY = y;
    haveLast = true;
  }
}

void drawWeatherMetricIcon(WeatherMetricKind kind, int cx, int cy, uint16_t color) {
  switch (kind) {
    case WeatherMetricKind::Location:
      tft.drawCircle(cx, cy - 4, 5, color);
      tft.fillCircle(cx, cy - 4, 2, color);
      tft.fillTriangle(cx - 5, cy - 1, cx + 5, cy - 1, cx, cy + 8, color);
      break;
    case WeatherMetricKind::Timezone:
      tft.drawCircle(cx, cy, 8, color);
      tft.drawFastVLine(cx, cy - 5, 6, color);
      tft.drawLine(cx, cy, cx + 5, cy + 3, color);
      tft.drawPixel(cx - 3, cy - 3, color);
      tft.drawPixel(cx + 3, cy - 3, color);
      tft.drawPixel(cx - 3, cy + 3, color);
      tft.drawPixel(cx + 3, cy + 3, color);
      break;
    case WeatherMetricKind::Bluetooth:
      {
        const int by = cy - 2;
        tft.drawFastVLine(cx, by - 5, 11, color);
        tft.drawLine(cx, by - 5, cx + 5, by - 2, color);
        tft.drawLine(cx + 5, by - 2, cx, by + 1, color);
        tft.drawLine(cx, by + 1, cx + 5, by + 5, color);
        tft.drawLine(cx + 5, by + 5, cx, by + 1, color);
        tft.drawLine(cx - 5, by - 3, cx + 5, by + 5, color);
        tft.drawLine(cx - 5, by + 3, cx + 5, by - 5, color);
      }
      break;
    case WeatherMetricKind::Wind:
      tft.drawFastHLine(cx - 9, cy - 5, 17, color);
      tft.drawFastHLine(cx - 5, cy, 18, color);
      tft.drawFastHLine(cx - 9, cy + 5, 12, color);
      tft.drawCircle(cx + 9, cy - 5, 3, color);
      break;
    case WeatherMetricKind::Sunrise:
      tft.drawFastHLine(cx - 10, cy + 6, 21, color);
      tft.drawCircle(cx, cy + 6, 7, color);
      tft.fillRect(cx - 9, cy + 7, 19, 8, COLOR_CHIP);
      tft.drawFastVLine(cx, cy - 8, 6, color);
      tft.drawLine(cx - 7, cy - 2, cx - 10, cy - 6, color);
      tft.drawLine(cx + 7, cy - 2, cx + 10, cy - 6, color);
      break;
    case WeatherMetricKind::Sunset:
      tft.drawFastHLine(cx - 10, cy + 1, 21, color);
      tft.drawCircle(cx, cy + 1, 6, color);
      tft.fillRect(cx - 8, cy - 8, 17, 8, COLOR_CHIP);
      tft.drawFastVLine(cx, cy + 5, 5, color);
      tft.drawLine(cx - 6, cy + 5, cx - 9, cy + 8, color);
      tft.drawLine(cx + 6, cy + 5, cx + 9, cy + 8, color);
      break;
    case WeatherMetricKind::Wifi:
      tft.fillCircle(cx, cy + 8, 2, color);
      drawWifiArc(cx, cy + 8, 6, color);
      drawWifiArc(cx, cy + 8, 11, color);
      drawWifiArc(cx, cy + 8, 16, color);
      break;
    case WeatherMetricKind::Date:
      tft.drawRoundRect(cx - 9, cy - 8, 18, 16, 2, color);
      tft.fillRect(cx - 8, cy - 7, 16, 4, color);
      tft.drawPixel(cx - 4, cy, color);
      tft.drawPixel(cx, cy, color);
      tft.drawPixel(cx + 4, cy, color);
      tft.drawPixel(cx - 4, cy + 4, color);
      tft.drawPixel(cx, cy + 4, color);
      tft.drawPixel(cx + 4, cy + 4, color);
      break;
  }
}

void drawWeatherTile(int x, int y, int w, int h, WeatherMetricKind kind,
                     const char *value, uint16_t color) {
  char fitted[16];
  fitDisplayText(fitted, sizeof(fitted), value, 8);

  tft.fillRoundRect(x, y, w, h, 4, COLOR_CHIP);
  tft.drawRoundRect(x, y, w, h, 4, color == ILI9341_DARKGREY ? COLOR_PANEL_2 : color);
  drawWeatherMetricIcon(kind, x + 17, y + h / 2, color);
  tft.setTextSize(2);
  tft.setTextColor(color, COLOR_CHIP);
  tft.setCursor(x + 36, y + 4);
  tft.print(fitted);
}

void drawCachedWeatherTile(DisplayCacheSlot &slot, int x, int y, WeatherMetricKind kind,
                           const char *value, uint16_t color) {
  char key[DISPLAY_CACHE_TEXT_LEN];
  snprintf(key, sizeof(key), "%d:%04X:%s", (int)kind, (unsigned)color, value ? value : "");
  if (!updateDisplayCacheSlot(slot, key)) return;
  drawWeatherTile(x, y, WEATHER_TILE_W, WEATHER_TILE_H, kind, value, color);
}

void drawRowLabel(int y, const char *label) {
  tft.fillCircle(17, y + 11, 3, COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_MUTED, COLOR_BG);
  tft.setCursor(27, y + 7);
  tft.print(label);
}

void drawRowValue(int y, const char *value, uint16_t color) {
  char fitted[24];
  fitDisplayText(fitted, sizeof(fitted), value, 14);

  tft.fillRoundRect(ROW_VALUE_X, y, ROW_VALUE_W, ROW_H, 5, COLOR_CHIP);
  tft.drawRoundRect(ROW_VALUE_X, y, ROW_VALUE_W, ROW_H, 5, color == ILI9341_DARKGREY ? COLOR_PANEL_2 : color);
  tft.setTextSize(2);
  tft.setTextColor(color, COLOR_CHIP);
  tft.setCursor(ROW_VALUE_X + 10, y + 3);
  tft.print(fitted);
}

void drawCachedRowValue(DisplayCacheSlot &slot, int y, const char *value, uint16_t color) {
  if (!updateTextColorCache(slot, value, color)) return;
  drawRowValue(y, value, color);
}

void drawPageDots() {
  const int startX = 38;
  const int y = 226;
  const int barW = 36;
  for (int i = 0; i < DISPLAY_PAGE_COUNT; i++) {
    const uint16_t color = state.displayPage == i ? ILI9341_CYAN : ILI9341_DARKGREY;
    tft.fillRoundRect(startX + i * 48, y, barW, 5, 2, color);
  }
}

void drawBannerDynamic() {
  char bannerKey[16];
  snprintf(bannerKey, sizeof(bannerKey), "%u:%u", state.alarm ? 1 : 0, state.displayPage);
  if (!updateDisplayCacheSlot(bannerCache, bannerKey)) return;

  tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, COLOR_BG);
  if (state.alarm) {
    tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, ILI9341_RED);
    tft.drawFastHLine(0, BANNER_Y, SCREEN_WIDTH, ILI9341_WHITE);
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(82, BANNER_Y + 5);
    tft.print("ALARM: NOISE!");
  } else {
    tft.drawFastHLine(0, BANNER_Y, SCREEN_WIDTH, COLOR_PANEL_2);
    drawPageDots();
  }
}

void drawCommonStatic(const char *title) {
  tft.fillScreen(COLOR_BG);
  drawHeaderStatic(title);
}

void drawHomeStatic() {
  drawCommonStatic("HOME");
  drawCardFrame(CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, "INDOOR TEMP");
  drawCardFrame(CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, "HUMIDITY");
  drawRowLabel(ROW_1_Y, "ROOM");
  drawRowLabel(ROW_1_Y + ROW_STEP, "SEC");
  drawRowLabel(ROW_1_Y + ROW_STEP * 2, "AC");
  drawRowLabel(ROW_1_Y + ROW_STEP * 3, "IR");
}

void drawHomeDynamic() {
  char tempText[16];
  char humText[16];
  snprintf(tempText, sizeof(tempText), "%.1fC", state.tempC);
  snprintf(humText, sizeof(humText), "%.0f%%", state.humidity);

  drawCachedCardValue(homeCache[SlotCardLeft], CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, tempText, ILI9341_CYAN);
  drawCachedCardValue(homeCache[SlotCardRight], CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, humText, ILI9341_GREEN);
  drawCachedRowValue(homeCache[SlotRow1], ROW_1_Y, state.presence ? "OCCUPIED" : "EMPTY", state.presence ? ILI9341_GREEN : ILI9341_ORANGE);
  drawCachedRowValue(homeCache[SlotRow2], ROW_1_Y + ROW_STEP, state.securityArmed ? "ARMED" : "OFF", state.securityArmed ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawCachedRowValue(homeCache[SlotRow3], ROW_1_Y + ROW_STEP * 2, state.acCooling ? "COOLING" : "STANDBY", state.acCooling ? ILI9341_CYAN : ILI9341_DARKGREY);
  drawCachedRowValue(homeCache[SlotRow4], ROW_1_Y + ROW_STEP * 3, state.irReceived ? "RX" : "IDLE", state.irReceived ? ILI9341_MAGENTA : ILI9341_DARKGREY);
}

void drawWeatherStatic() {
  drawCommonStatic("WEATHER");
  drawCardFrame(CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, "OUTDOOR");
  drawCardFrame(CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, "SKY");
}

void drawWeatherDynamic() {
  char outText[16];
  snprintf(outText, sizeof(outText), "%.1fC", state.outdoorTempC);
  WeatherDisplaySnapshot snapshot;
  snapshot.weatherReady = state.weatherReady;
  snapshot.locationReady = state.locationReady;
  snapshot.wifiConnected = state.wifiStaConnected;
  snapshot.bleConnected = state.bleClientConnected;
  snapshot.utcOffsetSeconds = state.utcOffsetSeconds;
  snapshot.outdoorTempC = state.outdoorTempC;
  snapshot.windKph = state.windKph;
  snapshot.locationText = state.locationText;
  snapshot.weatherText = state.weatherText;
  snapshot.sunriseText = state.sunriseText;
  snapshot.sunsetText = state.sunsetText;
  snapshot.dateText = state.dateText;
  char metric[16];

  drawCachedCardValue(weatherCache[SlotCardLeft], CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, state.weatherReady ? outText : "--", ILI9341_ORANGE);
  drawCachedWeatherIconCard(weatherCache[SlotCardRight], state.weatherReady ? state.weatherCode : -1,
                            state.weatherReady ? state.weatherText : "WAIT");

  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Location, snapshot);
  drawCachedWeatherTile(weatherCache[SlotRow1], CARD_LEFT_X, WEATHER_TILE_Y,
                        WeatherMetricKind::Location, metric,
                        state.locationReady ? ILI9341_GREEN : ILI9341_ORANGE);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Timezone, snapshot);
  drawCachedWeatherTile(weatherCache[SlotRow2], CARD_RIGHT_X, WEATHER_TILE_Y,
                        WeatherMetricKind::Timezone, metric, ILI9341_LIGHTGREY);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Sunrise, snapshot);
  drawCachedWeatherTile(weatherCache[SlotRow3], CARD_LEFT_X, WEATHER_TILE_Y + WEATHER_TILE_STEP_Y,
                        WeatherMetricKind::Sunrise, metric,
                        state.weatherReady ? ILI9341_YELLOW : ILI9341_DARKGREY);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Sunset, snapshot);
  drawCachedWeatherTile(weatherCache[SlotRow4], CARD_RIGHT_X, WEATHER_TILE_Y + WEATHER_TILE_STEP_Y,
                        WeatherMetricKind::Sunset, metric,
                        state.weatherReady ? ILI9341_MAGENTA : ILI9341_DARKGREY);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Wind, snapshot);
  drawCachedWeatherTile(weatherCache[SlotWeatherTile5], CARD_LEFT_X, WEATHER_TILE_Y + WEATHER_TILE_STEP_Y * 2,
                        WeatherMetricKind::Wind, metric,
                        state.weatherReady ? ILI9341_CYAN : ILI9341_DARKGREY);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Date, snapshot);
  drawCachedWeatherTile(weatherCache[SlotWeatherTile6], CARD_RIGHT_X, WEATHER_TILE_Y + WEATHER_TILE_STEP_Y * 2,
                        WeatherMetricKind::Date, metric, ILI9341_LIGHTGREY);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Wifi, snapshot);
  drawCachedWeatherTile(weatherCache[SlotWeatherTile7], CARD_LEFT_X, WEATHER_TILE_Y + WEATHER_TILE_STEP_Y * 3,
                        WeatherMetricKind::Wifi, metric,
                        state.wifiStaConnected ? ILI9341_GREEN : ILI9341_RED);
  formatWeatherMetricText(metric, sizeof(metric), WeatherMetricKind::Bluetooth, snapshot);
  drawCachedWeatherTile(weatherCache[SlotWeatherTile8], CARD_RIGHT_X, WEATHER_TILE_Y + WEATHER_TILE_STEP_Y * 3,
                        WeatherMetricKind::Bluetooth, metric,
                        state.bleClientConnected ? ILI9341_CYAN : ILI9341_DARKGREY);
}

void drawSystemStatic() {
  drawCommonStatic("SYSTEM");
  drawCardFrame(CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, "NOISE");
  drawCardFrame(CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, "BLE");
  drawRowLabel(ROW_1_Y, "SOUND");
  drawRowLabel(ROW_1_Y + ROW_STEP, "LIMIT");
  drawRowLabel(ROW_1_Y + ROW_STEP * 2, "LAMP");
  drawRowLabel(ROW_1_Y + ROW_STEP * 3, "IR TEST");
}

void drawSystemDynamic() {
  char micText[20];
  char thresholdText[20];
  snprintf(micText, sizeof(micText), "%d%%", noisePercentFor(state.micLevel, state.micThreshold));
  snprintf(thresholdText, sizeof(thresholdText), "%s", state.adaptiveMicReady ? "100%" : "CAL...");

  drawCachedCardValue(systemCache[SlotCardLeft], CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, micText, state.soundTriggered ? ILI9341_YELLOW : ILI9341_CYAN);
  drawCachedCardValue(systemCache[SlotCardRight], CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, state.bleClientConnected ? "CONNECTED" : "WAIT", state.bleClientConnected ? ILI9341_GREEN : ILI9341_DARKGREY);
  drawCachedRowValue(systemCache[SlotRow1], ROW_1_Y, state.soundTriggered ? "LOUD" : "QUIET", state.soundTriggered ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawCachedRowValue(systemCache[SlotRow2], ROW_1_Y + ROW_STEP, thresholdText, state.adaptiveMicReady ? ILI9341_GREEN : ILI9341_DARKGREY);
  drawCachedRowValue(systemCache[SlotRow3], ROW_1_Y + ROW_STEP * 2, state.lampOverride ? (state.manualLamp ? "MANUAL ON" : "MANUAL OFF") : "AUTO", state.lampOverride ? ILI9341_CYAN : ILI9341_GREEN);
  drawCachedRowValue(systemCache[SlotRow4], ROW_1_Y + ROW_STEP * 3, state.irTestActive ? "RUNNING" : "IDLE", state.irTestActive ? ILI9341_MAGENTA : ILI9341_DARKGREY);
}

void drawEventsStatic() {
  drawCommonStatic("AI GUARD");
  drawCardFrame(CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, "RISK SCORE");
  drawCardFrame(CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, "MIC LIMIT");
  drawRowLabel(ROW_1_Y, "LOG1");
  drawRowLabel(ROW_1_Y + ROW_STEP, "LOG2");
  drawRowLabel(ROW_1_Y + ROW_STEP * 2, "LOG3");
  drawRowLabel(ROW_1_Y + ROW_STEP * 3, "LOG4");
}

void drawEventsDynamic() {
  char riskText[20];
  char thresholdText[20];
  snprintf(riskText, sizeof(riskText), "%u %s", state.aiRiskScore, state.aiRiskText);
  snprintf(thresholdText, sizeof(thresholdText), "%s", state.adaptiveMicReady ? "100%" : "CAL...");

  const uint16_t riskColor = state.aiRiskScore >= 80 ? ILI9341_RED :
                             state.aiRiskScore >= 55 ? ILI9341_ORANGE :
                             state.aiRiskScore >= 30 ? ILI9341_YELLOW : ILI9341_GREEN;
  drawCachedCardValue(eventsCache[SlotCardLeft], CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, riskText, riskColor);
  drawCachedCardValue(eventsCache[SlotCardRight], CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, thresholdText, ILI9341_CYAN);
  for (uint8_t i = 0; i < HUB_EVENT_LOG_COUNT; i++) {
    char lineText[16];
    const char *line = eventLogLine(i);
    copyShortText(lineText, sizeof(lineText), line[0] ? line : "--");
    drawCachedRowValue(eventsCache[SlotRow1 + i], ROW_1_Y + i * ROW_STEP, lineText, ILI9341_WHITE);
  }
}

void drawXiaozhiStatic() {
  drawCommonStatic("XIAOZHI");
  drawCardFrame(CARD_LEFT_X, CARD_Y, CARD_W, CARD_H, "AI STATE");
  drawCardFrame(CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H, "SPEAKER");
  drawRowLabel(ROW_1_Y, "PROMPT");
  drawRowLabel(ROW_1_Y + ROW_STEP, "REPLY");
  drawRowLabel(ROW_1_Y + ROW_STEP * 2, "DOUBAO");
  drawRowLabel(ROW_1_Y + ROW_STEP * 3, "WAKE");
}

void drawXiaozhiDynamic() {
  const char *phase = xiaozhiPhaseName((XiaozhiPhase)state.xiaozhiPhase);
  const char *speaker = state.i2sSpeakerOk
      ? (state.speakerPlaying ? "PLAYING" : "READY")
      : "I2S ERR";
  drawCachedCardValue(xiaozhiCache[SlotCardLeft], CARD_LEFT_X, CARD_Y, CARD_W, CARD_H,
                      phase, state.xiaozhiPhase == (uint8_t)XiaozhiPhase::Idle ? ILI9341_GREEN : ILI9341_CYAN);
  drawCachedCardValue(xiaozhiCache[SlotCardRight], CARD_RIGHT_X, CARD_Y, CARD_W, CARD_H,
                      speaker, state.i2sSpeakerOk ? ILI9341_GREEN : ILI9341_RED);
  drawCachedRowValue(xiaozhiCache[SlotRow1], ROW_1_Y, state.xiaozhiPromptText, ILI9341_LIGHTGREY);
  drawCachedRowValue(xiaozhiCache[SlotRow2], ROW_1_Y + ROW_STEP, state.xiaozhiReplyText, ILI9341_WHITE);
  drawCachedRowValue(xiaozhiCache[SlotRow3], ROW_1_Y + ROW_STEP * 2,
                     state.doubaoRelayConnected ? "DOUBAO READY" : "DOUBAO OFFLINE",
                     state.doubaoRelayConnected ? ILI9341_CYAN : ILI9341_ORANGE);
  drawCachedRowValue(xiaozhiCache[SlotRow4], ROW_1_Y + ROW_STEP * 3,
                     state.xiaozhiAutoWake ? "MIC AUTO" : "MANUAL",
                     state.xiaozhiAutoWake ? ILI9341_GREEN : ILI9341_DARKGREY);
}

void drawStaticForPage(uint8_t page) {
  switch (page) {
    case 4:
      drawXiaozhiStatic();
      break;
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
    case 4:
      drawXiaozhiDynamic();
      break;
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
