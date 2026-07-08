#include "pcm_ring_buffer.h"

PcmRingBuffer::PcmRingBuffer(int16_t *storage, size_t capacity)
    : storage_(storage),
      capacity_(storage ? capacity : 0),
      start_(0),
      count_(0),
      overflowed_(false) {}

size_t PcmRingBuffer::push(const int16_t *samples, size_t count) {
  if (!storage_ || !samples || capacity_ == 0 || count == 0) {
    return 0;
  }

  const size_t accepted = count < free() ? count : free();
  if (accepted < count) {
    overflowed_ = true;
  }
  for (size_t i = 0; i < accepted; ++i) {
    storage_[(start_ + count_ + i) % capacity_] = samples[i];
  }
  count_ += accepted;
  return accepted;
}

size_t PcmRingBuffer::pop(int16_t *out, size_t outCapacity) {
  if (!storage_ || capacity_ == 0 || !out || outCapacity == 0) {
    return 0;
  }

  const size_t popped = count_ < outCapacity ? count_ : outCapacity;
  for (size_t i = 0; i < popped; ++i) {
    out[i] = storage_[(start_ + i) % capacity_];
  }
  start_ = (start_ + popped) % capacity_;
  count_ -= popped;
  if (count_ == 0) {
    start_ = 0;
  }
  return popped;
}

size_t PcmRingBuffer::size() const {
  return count_;
}

size_t PcmRingBuffer::free() const {
  return capacity_ - count_;
}

bool PcmRingBuffer::empty() const {
  return count_ == 0;
}

void PcmRingBuffer::clear() {
  start_ = 0;
  count_ = 0;
  overflowed_ = false;
}

bool PcmRingBuffer::overflowed() const {
  return overflowed_;
}
