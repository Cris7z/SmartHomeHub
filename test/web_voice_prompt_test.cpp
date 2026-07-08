#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
  std::ifstream input("src/net/web_dashboard.cpp");
  assert(input.good());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string source = buffer.str();

  assert(source.find("SpeechRecognition") != std::string::npos);
  assert(source.find("listenXiaoZhi") != std::string::npos);
  assert(source.find("speechSynthesis") != std::string::npos);
  assert(source.find("speakXiaoZhiReply") != std::string::npos);
  assert(source.find("waitXiaoZhiReply") != std::string::npos);
  assert(source.find("prompt=") != std::string::npos);
  assert(source.find("triggerXiaozhiAiWithPrompt") != std::string::npos);
  return 0;
}
