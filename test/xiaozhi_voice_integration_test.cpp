#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
  std::ifstream input("src/app/xiaozhi_ai.cpp");
  assert(input.good());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string source = buffer.str();

  assert(source.find("queueSpeakerPcmClip") != std::string::npos);
  assert(source.find("XIAOZHI_HELLO_PCM") != std::string::npos);
  assert(source.find("queueSpeakerTone") != std::string::npos);
  assert(source.find("xiaozhiMicSuppressed") != std::string::npos);
  assert(source.find("xiaozhiMicMutedUntilMs") != std::string::npos);
  assert(source.find("XIAOZHI_MIC_COOLDOWN_AFTER_SPEAK_MS") != std::string::npos);
  return 0;
}
