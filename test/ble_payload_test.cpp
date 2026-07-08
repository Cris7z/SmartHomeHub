#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

int main() {
  std::ifstream input("src/net/ble_service.cpp");
  assert(input.good());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string source = buffer.str();

  assert(source.find("\"\\\"events\\\":[\"") == std::string::npos);
  assert(source.find("\"\\\"timezone\\\":") == std::string::npos);
  assert(source.find("\"\\\"sunrise\\\":") == std::string::npos);
  assert(source.find("\"\\\"sunset\\\":") == std::string::npos);
  assert(source.find("\"\\\"xiaozhiPrompt\\\":") == std::string::npos);
  assert(source.find("\"\\\"xiaozhiReply\\\":") == std::string::npos);
  return 0;
}
