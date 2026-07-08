#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
  std::ifstream input("src/board/config.h");
  assert(input.good());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string source = buffer.str();

  assert(source.find("constexpr bool XIAOZHI_AUTO_WAKE_ENABLED = true;") != std::string::npos);
  assert(source.find("constexpr int MIC_RMS_TRIGGER = 30000;") != std::string::npos);
  assert(source.find("constexpr int MIC_RMS_ADAPT_MARGIN = 6000;") != std::string::npos);
  assert(source.find("constexpr int MIC_RMS_ADAPT_MULTIPLIER_PERCENT = 120;") != std::string::npos);
  assert(source.find("constexpr uint8_t VOICE_PCM_RIGHT_SHIFT = 16;") != std::string::npos);
  assert(source.find("constexpr uint32_t XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS = 6000;") != std::string::npos);
  assert(source.find("constexpr uint32_t XIAOZHI_AUTO_REARM_QUIET_MS = 1200;") != std::string::npos);

  std::ifstream aiInput("src/app/xiaozhi_ai.cpp");
  assert(aiInput.good());
  std::ostringstream aiBuffer;
  aiBuffer << aiInput.rdbuf();
  const std::string aiSource = aiBuffer.str();
  assert(aiSource.find("lastAutoSoundTriggered") != std::string::npos);
  assert(aiSource.find("autoSoundRising") != std::string::npos);
  assert(aiSource.find("autoWakeArmed") != std::string::npos);
  assert(aiSource.find("XIAOZHI_AUTO_REARM_QUIET_MS") != std::string::npos);
  return 0;
}
