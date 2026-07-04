#include "display_cache.h"

#include <cstring>

namespace {
void copyCacheText(char *out, std::size_t outSize, const char *value) {
  if (!out || outSize == 0) return;
  const char *safeValue = value ? value : "";
  std::strncpy(out, safeValue, outSize - 1);
  out[outSize - 1] = '\0';
}
}

bool updateDisplayCacheSlot(DisplayCacheSlot &slot, const char *value) {
  const char *safeValue = value ? value : "";
  if (slot.valid && std::strncmp(slot.value, safeValue, DISPLAY_CACHE_TEXT_LEN) == 0) {
    return false;
  }

  copyCacheText(slot.value, sizeof(slot.value), safeValue);
  slot.valid = true;
  return true;
}

void resetDisplayCacheSlot(DisplayCacheSlot &slot) {
  slot.valid = false;
  slot.value[0] = '\0';
}

void resetDisplayCacheSlots(DisplayCacheSlot *slots, std::size_t count) {
  if (!slots) return;
  for (std::size_t i = 0; i < count; i++) {
    resetDisplayCacheSlot(slots[i]);
  }
}
