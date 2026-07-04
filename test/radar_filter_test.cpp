#include <cassert>
#include <cstdint>

#include "../src/io/radar_filter.h"

int main() {
  RadarPresenceFilter filter;

  assert(!filter.update(false, 0));
  assert(filter.update(true, 10));

  // A brief OUT drop should not make the room look empty.
  const uint32_t lowStartMs = 100;
  assert(filter.update(false, lowStartMs));
  assert(filter.update(false, lowStartMs + RADAR_EMPTY_HOLD_MS - 1));

  // Once the low level lasts long enough, the stable presence state clears.
  assert(!filter.update(false, lowStartMs + RADAR_EMPTY_HOLD_MS + 20));

  // The next high sample should mark the room occupied immediately.
  assert(filter.update(true, lowStartMs + RADAR_EMPTY_HOLD_MS + 30));

  return 0;
}
