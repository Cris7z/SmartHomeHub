#include "voice_capture_core.h"

#include <climits>

namespace {

uint64_t absMagnitude(int32_t value) {
  const int64_t wide = value;
  return static_cast<uint64_t>(wide < 0 ? -wide : wide);
}

int16_t clampToPcm16(int32_t value) {
  if (value > INT16_MAX) {
    return INT16_MAX;
  }
  if (value < INT16_MIN) {
    return INT16_MIN;
  }
  return static_cast<int16_t>(value);
}

}  // namespace

int selectVoiceI2sSlot(const int32_t *samples, size_t sampleCount) {
  if (!samples || sampleCount == 0) {
    return 0;
  }

  uint64_t totals[2] = {0, 0};
  for (size_t i = 0; i < sampleCount; ++i) {
    totals[i & 1] += absMagnitude(samples[i]);
  }
  return totals[1] > totals[0] ? 1 : 0;
}

size_t convertVoiceI2sToPcm16(const int32_t *samples, size_t sampleCount,
                              uint8_t slot, uint8_t rightShift,
                              int16_t *out, size_t outCapacity) {
  if (!samples || !out || outCapacity == 0 || slot > 1) {
    return 0;
  }

  size_t written = 0;
  for (size_t i = slot; i < sampleCount && written < outCapacity; i += 2) {
    const int32_t shifted =
        rightShift >= 31 ? (samples[i] < 0 ? -1 : 0) : (samples[i] >> rightShift);
    out[written++] = clampToPcm16(shifted);
  }
  return written;
}

VoicePreRoll::VoicePreRoll(int16_t *storage, size_t capacity)
    : storage_(storage), capacity_(storage ? capacity : 0), start_(0), count_(0) {}

void VoicePreRoll::clear() {
  start_ = 0;
  count_ = 0;
}

void VoicePreRoll::push(const int16_t *samples, size_t count) {
  if (!storage_ || capacity_ == 0 || !samples) {
    return;
  }

  for (size_t i = 0; i < count; ++i) {
    if (count_ < capacity_) {
      storage_[(start_ + count_) % capacity_] = samples[i];
      ++count_;
    } else {
      storage_[start_] = samples[i];
      start_ = (start_ + 1) % capacity_;
    }
  }
}

size_t VoicePreRoll::copyChronological(int16_t *out, size_t outCapacity) const {
  if (!out || outCapacity == 0 || !storage_) {
    return 0;
  }

  const size_t copied = count_ < outCapacity ? count_ : outCapacity;
  for (size_t i = 0; i < copied; ++i) {
    out[i] = storage_[(start_ + i) % capacity_];
  }
  return copied;
}

size_t VoicePreRoll::size() const {
  return count_;
}
