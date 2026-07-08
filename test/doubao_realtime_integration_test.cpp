#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

std::string readFile(const char *path) {
  std::ifstream file(path);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int main() {
  const std::string ai = readFile("src/app/xiaozhi_ai.cpp");
  const std::string mainSource = readFile("src/main.cpp");
  const std::string dashboard = readFile("src/net/web_dashboard.cpp");
  const std::string secrets = readFile("src/net/secrets_config.h");

  assert(ai.find("startVoiceStreamTurn") != std::string::npos);
  assert(ai.find("voiceStreamTurnActive") != std::string::npos);
  assert(mainSource.find("updateVoiceStreamClient") != std::string::npos);
  assert(mainSource.find("updateStreamingSpeaker") != std::string::npos);
  assert(dashboard.find("speechSynthesis.speak") == std::string::npos);
  assert(secrets.find("SMART_HOME_DOUBAO_RELAY_URL") != std::string::npos);
}
