#include <cassert>

#include "../src/app/xiaozhi_core.h"

int main() {
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::TurnStarted) ==
         XiaozhiPhase::Listening);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AsrEnded) ==
         XiaozhiPhase::Thinking);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AudioStarted) ==
         XiaozhiPhase::Speaking);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::TurnDone) ==
         XiaozhiPhase::Idle);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::Failed) ==
         XiaozhiPhase::Idle);
}
