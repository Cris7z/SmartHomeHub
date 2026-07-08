#include <cassert>
#include <cstdint>

#include "../src/board/pcm_ring_buffer.h"

int main() {
  int16_t storage[5] = {};
  PcmRingBuffer queue(storage, 5);
  const int16_t a[] = {1, 2, 3, 4};
  const int16_t b[] = {5, 6};
  assert(queue.push(a, 4) == 4);
  assert(queue.push(b, 2) == 1);
  assert(queue.overflowed());
  int16_t out[5] = {};
  assert(queue.pop(out, 5) == 5);
  for (int i = 0; i < 5; ++i) {
    assert(out[i] == i + 1);
  }
  assert(queue.empty());

  PcmRingBuffer empty(nullptr, 0);
  assert(empty.push(a, 1) == 0);
  assert(empty.pop(out, 1) == 0);
}
