#pragma once

#include <cstdint>

enum class ShortcutMacroKind {
  Home,
  Away,
  Night
};

struct ShortcutMacroPreset {
  bool forceSecurity = false;
  bool alarm = false;
  bool lampOverride = false;
  bool manualLamp = false;
  bool manualAc = false;
  uint8_t displayPage = 0;
  const char *eventText = "";
  const char *serialName = "";
};

ShortcutMacroPreset shortcutMacroPreset(ShortcutMacroKind kind);
const char *shortcutMacroCommandName(ShortcutMacroKind kind);
