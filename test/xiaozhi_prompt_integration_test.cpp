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

  assert(source.find("triggerXiaozhiAiWithPrompt") != std::string::npos);
  assert(source.find("const char *effectivePrompt") != std::string::npos);
  assert(source.find("buildLocalReply") != std::string::npos);
  assert(source.find("strlcpy(state.xiaozhiPromptText, \"Home status request\"") == std::string::npos);
  return 0;
}
