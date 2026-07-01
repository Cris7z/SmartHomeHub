#include "display.h"

#include <Arduino.h>

#include "hub_state.h"
#include "../board/hardware.h"

namespace {
uint32_t lastUiDrawMs = 0;

void drawStaticUi() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 320, 34, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.setCursor(12, 9);
  tft.print("Lexin Smart Home");

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_LIGHTGREY, ILI9341_BLACK);
  tft.setCursor(18, 115);
  tft.print("ROOM");
  tft.setCursor(18, 150);
  tft.print("SEC");
  tft.setCursor(18, 185);
  tft.print("AC");
  tft.setCursor(18, 220);
  tft.print("IR");
}

void drawStatusRow(int y, const char *label, const char *value, uint16_t color) {
  (void)label;
  tft.fillRect(118, y - 8, 190, 32, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextSize(3);
  tft.setTextColor(color, ILI9341_BLACK);
  tft.setCursor(118, y - 5);
  tft.print(value);
}
}

void setupDisplay() {
  tft.begin();
  tft.setRotation(1);
  drawStaticUi();
}

void drawUi() {
  if (millis() - lastUiDrawMs < 500) return;
  lastUiDrawMs = millis();

  tft.fillCircle(302, 17, 7, ILI9341_NAVY);
  tft.fillCircle(302, 17, 6, (millis() / 500) % 2 == 0 ? ILI9341_CYAN : ILI9341_DARKGREY);

  tft.fillRect(18, 52, 145, 36, ILI9341_BLACK);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setTextSize(4);
  tft.setCursor(18, 52);
  tft.printf("%.1fC", state.tempC);

  tft.fillRect(180, 52, 120, 36, ILI9341_BLACK);
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

  tft.fillRect(0, 246, 320, 24, ILI9341_BLACK);
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
