#pragma once

#include <cstddef>

constexpr std::size_t DISPLAY_CACHE_TEXT_LEN = 48;

struct DisplayCacheSlot {
  bool valid = false;
  char value[DISPLAY_CACHE_TEXT_LEN] = "";
};

bool updateDisplayCacheSlot(DisplayCacheSlot &slot, const char *value);
void resetDisplayCacheSlot(DisplayCacheSlot &slot);
void resetDisplayCacheSlots(DisplayCacheSlot *slots, std::size_t count);
