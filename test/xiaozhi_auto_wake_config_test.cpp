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
  return 0;
}
