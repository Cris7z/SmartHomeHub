#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
  std::ifstream input("src/main.cpp");
  assert(input.good());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string source = buffer.str();

  const std::size_t clearRgb = source.find("clearOnboardRgb();");
  const std::size_t serialBegin = source.find("Serial.begin");

  assert(clearRgb != std::string::npos);
  assert(serialBegin != std::string::npos);
  assert(clearRgb < serialBegin);
  return 0;
}
