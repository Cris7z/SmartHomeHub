#include <cassert>
#include <cstring>

#include "src/app/shortcut_macro.h"

namespace {
void testHomeMacroReturnsToAutoHomePage() {
  const ShortcutMacroPreset preset = shortcutMacroPreset(ShortcutMacroKind::Home);

  assert(!preset.forceSecurity);
  assert(!preset.alarm);
  assert(!preset.lampOverride);
  assert(!preset.manualLamp);
  assert(!preset.manualAc);
  assert(preset.displayPage == 0);
  assert(std::strcmp(preset.eventText, "MACRO home") == 0);
  assert(std::strcmp(shortcutMacroCommandName(ShortcutMacroKind::Home), "home") == 0);
}

void testAwayMacroArmsSecurityAndTurnsLampOff() {
  const ShortcutMacroPreset preset = shortcutMacroPreset(ShortcutMacroKind::Away);

  assert(preset.forceSecurity);
  assert(!preset.alarm);
  assert(preset.lampOverride);
  assert(!preset.manualLamp);
  assert(!preset.manualAc);
  assert(preset.displayPage == 3);
  assert(std::strcmp(preset.eventText, "MACRO away") == 0);
  assert(std::strcmp(shortcutMacroCommandName(ShortcutMacroKind::Away), "away") == 0);
}

void testNightMacroUsesWeatherClockPage() {
  const ShortcutMacroPreset preset = shortcutMacroPreset(ShortcutMacroKind::Night);

  assert(preset.forceSecurity);
  assert(!preset.alarm);
  assert(preset.lampOverride);
  assert(!preset.manualLamp);
  assert(!preset.manualAc);
  assert(preset.displayPage == 1);
  assert(std::strcmp(preset.eventText, "MACRO night") == 0);
  assert(std::strcmp(shortcutMacroCommandName(ShortcutMacroKind::Night), "night") == 0);
}
}

int main() {
  testHomeMacroReturnsToAutoHomePage();
  testAwayMacroArmsSecurityAndTurnsLampOff();
  testNightMacroUsesWeatherClockPage();
  return 0;
}
