#include "radar_filter.h"

bool RadarPresenceFilter::update(bool rawOccupied, uint32_t nowMs) {
  if (rawOccupied) {
    occupied_ = true;
    emptyCandidateActive_ = false;
    return occupied_;
  }

  if (!occupied_) {
    emptyCandidateActive_ = false;
    return occupied_;
  }

  if (!emptyCandidateActive_) {
    emptyCandidateActive_ = true;
    emptyCandidateSinceMs_ = nowMs;
  }

  if (nowMs - emptyCandidateSinceMs_ >= RADAR_EMPTY_HOLD_MS) {
    occupied_ = false;
    emptyCandidateActive_ = false;
  }

  return occupied_;
}
