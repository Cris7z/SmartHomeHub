#include <cassert>
#include <cstring>

#include "../src/app/display_text.h"

int main() {
  char out[16];

  fitDisplayText(out, sizeof(out), "CONNECTED", 12);
  assert(std::strcmp(out, "CONNECTED") == 0);

  fitDisplayText(out, sizeof(out), "VeryLongCityName", 10);
  assert(std::strcmp(out, "VeryLong..") == 0);

  fitDisplayText(out, sizeof(out), "ABCDE", 2);
  assert(std::strcmp(out, "AB") == 0);

  fitDisplayText(out, sizeof(out), nullptr, 8);
  assert(std::strcmp(out, "") == 0);

  return 0;
}
