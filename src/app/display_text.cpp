#include "display_text.h"

#include <cstring>

void fitDisplayText(char *out, std::size_t outSize, const char *text, std::size_t maxChars) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  if (!text || maxChars == 0) return;

  const std::size_t sourceLen = std::strlen(text);
  std::size_t copyLen = sourceLen;
  bool truncated = false;
  if (copyLen > maxChars) {
    copyLen = maxChars;
    truncated = true;
  }
  if (copyLen >= outSize) {
    copyLen = outSize - 1;
    truncated = sourceLen > copyLen;
  }

  if (truncated && copyLen >= 4) {
    copyLen -= 2;
    std::memcpy(out, text, copyLen);
    out[copyLen++] = '.';
    out[copyLen++] = '.';
    out[copyLen] = '\0';
    return;
  }

  std::memcpy(out, text, copyLen);
  out[copyLen] = '\0';
}
