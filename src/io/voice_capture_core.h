#pragma once

#include <cstddef>
#include <cstdint>

int selectVoiceI2sSlot(const int32_t *samples, size_t sampleCount);
size_t convertVoiceI2sToPcm16(const int32_t *samples, size_t sampleCount,
                              uint8_t slot, uint8_t rightShift,
                              int16_t *out, size_t outCapacity);

class VoicePreRoll {
 public:
  VoicePreRoll(int16_t *storage, size_t capacity);

  void clear();
  void push(const int16_t *samples, size_t count);
  size_t copyChronological(int16_t *out, size_t outCapacity) const;
  size_t size() const;

 private:
  int16_t *storage_;
  size_t capacity_;
  size_t start_;
  size_t count_;
};
