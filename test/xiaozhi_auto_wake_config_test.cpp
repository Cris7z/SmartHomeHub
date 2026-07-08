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

  assert(source.find("constexpr bool XIAOZHI_AUTO_WAKE_ENABLED = false;") != std::string::npos);
  return 0;
}
