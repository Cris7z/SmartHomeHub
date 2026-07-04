#include <cassert>

#include "../src/app/display_cache.h"

int main() {
  DisplayCacheSlot slot;

  assert(updateDisplayCacheSlot(slot, "WAIT"));
  assert(!updateDisplayCacheSlot(slot, "WAIT"));
  assert(updateDisplayCacheSlot(slot, "READY"));
  assert(!updateDisplayCacheSlot(slot, "READY"));

  DisplayCacheSlot slots[2];
  assert(updateDisplayCacheSlot(slots[0], "A"));
  assert(updateDisplayCacheSlot(slots[1], "B"));
  resetDisplayCacheSlots(slots, 2);
  assert(updateDisplayCacheSlot(slots[0], "A"));
  assert(updateDisplayCacheSlot(slots[1], "B"));

  return 0;
}
