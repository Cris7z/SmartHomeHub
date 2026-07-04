#pragma once

#include <stdint.h>

constexpr uint32_t RADAR_EMPTY_HOLD_MS = 2500;

class RadarPresenceFilter {
 public:
  bool update(bool rawOccupied, uint32_t nowMs);
  bool occupied() const { return occupied_; }

 private:
  bool occupied_ = false;
  bool emptyCandidateActive_ = false;
  uint32_t emptyCandidateSinceMs_ = 0;
};
