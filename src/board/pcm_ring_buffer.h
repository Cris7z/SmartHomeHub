#pragma once

#include <cstddef>
#include <cstdint>

class PcmRingBuffer {
 public:
  PcmRingBuffer(int16_t *storage = nullptr, size_t capacity = 0);

  size_t push(const int16_t *samples, size_t count);
  size_t pop(int16_t *out, size_t outCapacity);
  size_t size() const;
  size_t free() const;
  bool empty() const;
  void clear();
  bool overflowed() const;

 private:
  int16_t *storage_;
  size_t capacity_;
  size_t start_;
  size_t count_;
  bool overflowed_;
};
