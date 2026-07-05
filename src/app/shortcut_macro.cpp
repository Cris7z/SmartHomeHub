#include "shortcut_macro.h"

namespace {
constexpr uint8_t PAGE_HOME = 0;
constexpr uint8_t PAGE_WEATHER = 1;
constexpr uint8_t PAGE_AI_GUARD = 3;

ShortcutMacroPreset makePreset(
  bool forceSecurity,
  bool alarm,
  bool lampOverride,
  bool manualLamp,
  bool manualAc,
  uint8_t displayPage,
  const char *eventText,
  const char *serialName
) {
  ShortcutMacroPreset preset;
  preset.forceSecurity = forceSecurity;
  preset.alarm = alarm;
  preset.lampOverride = lampOverride;
  preset.manualLamp = manualLamp;
  preset.manualAc = manualAc;
  preset.displayPage = displayPage;
  preset.eventText = eventText;
  preset.serialName = serialName;
  return preset;
}
}

ShortcutMacroPreset shortcutMacroPreset(ShortcutMacroKind kind) {
  switch (kind) {
    case ShortcutMacroKind::Home:
      return makePreset(
        false,
        false,
        false,
        false,
        false,
        PAGE_HOME,
        "MACRO home",
        "home"
      );
    case ShortcutMacroKind::Away:
      return makePreset(
        true,
        false,
        true,
        false,
        false,
        PAGE_AI_GUARD,
        "MACRO away",
        "away"
      );
    case ShortcutMacroKind::Night:
      return makePreset(
        true,
        false,
        true,
        false,
        false,
        PAGE_WEATHER,
        "MACRO night",
        "night"
      );
  }

  return shortcutMacroPreset(ShortcutMacroKind::Home);
}

const char *shortcutMacroCommandName(ShortcutMacroKind kind) {
  return shortcutMacroPreset(kind).serialName;
}
