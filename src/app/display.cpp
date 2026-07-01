#include "display.h"

#include <Arduino.h>

#include "hub_state.h"
#include "../board/hardware.h"

namespace {
uint32_t lastUiDrawMs = 0;

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr uint32_t UI_REFRESH_MS = 250;
constexpr uint32_t HEARTBEAT_MS = 500;
constexpr int HEADER_HEIGHT = 30;
constexpr int HEARTBEAT_X = 302;
constexpr int HEARTBEAT_Y = 15;
constexpr int TEMP_Y = 46;
constexpr int ROOM_ROW_Y = 94;
constexpr int SEC_ROW_Y = 126;
constexpr int AC_ROW_Y = 158;
constexpr int IR_ROW_Y = 190;
constexpr int BANNER_Y = 214;
constexpr int BANNER_HEIGHT = SCREEN_HEIGHT - BANNER_Y;

void drawStaticUi() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setCursor(12, 7);
  tft.print("Lexin Smart Home");

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
  tft.setCursor(18, ROOM_ROW_Y);
  tft.print("ROOM");
  tft.setCursor(18, SEC_ROW_Y);
  tft.print("SEC");
  tft.setCursor(18, AC_ROW_Y);
  tft.print("AC");
  tft.setCursor(18, IR_ROW_Y);
  tft.print("IR");
}

void drawStatusRow(int y, const char *label, const char *value, uint16_t color) {
  (void)label;
  tft.fillRect(118, y - 3, 190, 22, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(color, ILI9341_BLACK);
  tft.setCursor(118, y);
  tft.print(value);
}
}

void setupDisplay() {
  tft.begin();
  tft.setRotation(1);
  drawStaticUi();
}

void drawUi() {
  if (millis() - lastUiDrawMs < UI_REFRESH_MS) return;
  lastUiDrawMs = millis();

  tft.fillCircle(HEARTBEAT_X, HEARTBEAT_Y, 7, ILI9341_NAVY);
  tft.fillCircle(HEARTBEAT_X, HEARTBEAT_Y, 6, (millis() / HEARTBEAT_MS) % 2 == 0 ? ILI9341_CYAN : ILI9341_DARKGREY);

  tft.fillRect(18, TEMP_Y, 145, 28, ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(3);
  tft.setCursor(18, TEMP_Y);
  tft.printf("%.1fC", state.tempC);

  tft.fillRect(190, TEMP_Y, 105, 28, ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setCursor(190, TEMP_Y);
  tft.printf("%.0f%%", state.humidity);

  drawStatusRow(ROOM_ROW_Y, "ROOM", state.presence ? "OCCUPIED" : "EMPTY",
                state.presence ? ILI9341_GREEN : ILI9341_ORANGE);
  drawStatusRow(SEC_ROW_Y, "SEC", state.securityArmed ? "ARMED" : "OFF",
                state.securityArmed ? ILI9341_YELLOW : ILI9341_DARKGREY);
  drawStatusRow(AC_ROW_Y, "AC", state.acCooling ? "COOLING" : "STANDBY",
                state.acCooling ? ILI9341_CYAN : ILI9341_DARKGREY);
  drawStatusRow(IR_ROW_Y, "IR", state.irReceived ? "RX" : "IDLE",
                state.irReceived ? ILI9341_MAGENTA : ILI9341_DARKGREY);

  tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, ILI9341_BLACK);
  if (state.alarm) {
    tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(82, BANNER_Y + 5);
    tft.print("ALARM: NOISE!");
  } else if (state.forceSecurity) {
    tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, ILI9341_DARKGREEN);
    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(34, BANNER_Y + 5);
    tft.print("MODE: FORCED SECURITY");
  } else if (state.manualLamp || state.manualAc) {
    tft.fillRect(0, BANNER_Y, SCREEN_WIDTH, BANNER_HEIGHT, ILI9341_DARKCYAN);
    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
    tft.setTextSize(2);
    tft.setCursor(54, BANNER_Y + 5);
    tft.print("MANUAL: ");
    if (state.manualLamp) tft.print("LAMP ");
    if (state.manualAc) tft.print("AC");
  }
}
