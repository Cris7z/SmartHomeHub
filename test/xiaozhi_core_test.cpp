#include <cassert>
#include <cstring>

#include "../src/app/xiaozhi_core.h"

namespace {
void testPhaseNamesAreStableForUiAndJson() {
  assert(std::strcmp(xiaozhiPhaseName(XiaozhiPhase::Idle), "IDLE") == 0);
  assert(std::strcmp(xiaozhiPhaseName(XiaozhiPhase::Listening), "LISTEN") == 0);
  assert(std::strcmp(xiaozhiPhaseName(XiaozhiPhase::Thinking), "THINK") == 0);
  assert(std::strcmp(xiaozhiPhaseName(XiaozhiPhase::Speaking), "SPEAK") == 0);
}

void testAutoTriggerUsesSoundAndCooldown() {
  assert(xiaozhiShouldAutoTrigger(true, true, 1000, 0, 5000));
  assert(!xiaozhiShouldAutoTrigger(false, true, 1000, 0, 5000));
  assert(!xiaozhiShouldAutoTrigger(true, false, 1000, 0, 5000));
  assert(!xiaozhiShouldAutoTrigger(true, true, 4000, 1000, 5000));
  assert(xiaozhiShouldAutoTrigger(true, true, 7000, 1000, 5000));
}

void testAutoTriggerIsSuppressedDuringSpeakerPlaybackAndCooldown() {
  assert(xiaozhiMicSuppressed(true, 1000, 0));
  assert(xiaozhiMicSuppressed(false, 1000, 3500));
  assert(!xiaozhiMicSuppressed(false, 3500, 3500));

  assert(!xiaozhiShouldAutoTrigger(true, true, true, 7000, 1000, 5000));
  assert(xiaozhiShouldAutoTrigger(true, true, false, 7000, 1000, 5000));
}

void testLocalReplyContainsCurrentHomeSnapshot() {
  char reply[96] = {};
  formatXiaozhiLocalReply(reply, sizeof(reply), 26.5f, 55.0f, true, false);

  assert(std::strstr(reply, "26.5C") != nullptr);
  assert(std::strstr(reply, "55%") != nullptr);
  assert(std::strstr(reply, "OCCUPIED") != nullptr);
  assert(std::strstr(reply, "SEC OFF") != nullptr);
}
}

int main() {
  testPhaseNamesAreStableForUiAndJson();
  testAutoTriggerUsesSoundAndCooldown();
  testAutoTriggerIsSuppressedDuringSpeakerPlaybackAndCooldown();
  testLocalReplyContainsCurrentHomeSnapshot();
  return 0;
}
