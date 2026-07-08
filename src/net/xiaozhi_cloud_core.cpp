#include "xiaozhi_cloud_core.h"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
bool hasValue(const char *value) {
  return value && value[0] != '\0';
}

bool equalsIgnoreCase(const char *left, const char *right) {
  if (!left || !right) {
    return false;
  }

  while (*left && *right) {
    const unsigned char a = (unsigned char)*left++;
    const unsigned char b = (unsigned char)*right++;
    if (std::tolower(a) != std::tolower(b)) {
      return false;
    }
  }
  return *left == '\0' && *right == '\0';
}

bool appendRaw(char *out, size_t outSize, size_t &pos, const char *text) {
  if (!out || outSize == 0 || !text) {
    return false;
  }

  while (*text) {
    if (pos + 1 >= outSize) {
      out[outSize - 1] = '\0';
      return false;
    }
    out[pos++] = *text++;
  }
  out[pos] = '\0';
  return true;
}

bool appendChar(char *out, size_t outSize, size_t &pos, char value) {
  if (!out || outSize == 0 || pos + 1 >= outSize) {
    if (out && outSize > 0) {
      out[outSize - 1] = '\0';
    }
    return false;
  }
  out[pos++] = value;
  out[pos] = '\0';
  return true;
}

bool appendJsonEscaped(char *out, size_t outSize, size_t &pos, const char *text) {
  if (!text) {
    return true;
  }

  while (*text) {
    const unsigned char c = (unsigned char)*text++;
    if (c == '"' || c == '\\') {
      if (!appendChar(out, outSize, pos, '\\') ||
          !appendChar(out, outSize, pos, (char)c)) {
        return false;
      }
    } else if (c == '\n') {
      if (!appendRaw(out, outSize, pos, "\\n")) return false;
    } else if (c == '\r') {
      if (!appendRaw(out, outSize, pos, "\\r")) return false;
    } else if (c == '\t') {
      if (!appendRaw(out, outSize, pos, "\\t")) return false;
    } else if (c < 0x20) {
      char escaped[7] = {};
      std::snprintf(escaped, sizeof(escaped), "\\u%04x", c);
      if (!appendRaw(out, outSize, pos, escaped)) return false;
    } else if (!appendChar(out, outSize, pos, (char)c)) {
      return false;
    }
  }
  return true;
}

bool appendUtf8(char *out, size_t outSize, size_t &pos, uint32_t codepoint) {
  if (codepoint <= 0x7F) {
    return appendChar(out, outSize, pos, (char)codepoint);
  }
  if (codepoint <= 0x7FF) {
    return appendChar(out, outSize, pos, (char)(0xC0 | (codepoint >> 6))) &&
           appendChar(out, outSize, pos, (char)(0x80 | (codepoint & 0x3F)));
  }
  return appendChar(out, outSize, pos, (char)(0xE0 | (codepoint >> 12))) &&
         appendChar(out, outSize, pos, (char)(0x80 | ((codepoint >> 6) & 0x3F))) &&
         appendChar(out, outSize, pos, (char)(0x80 | (codepoint & 0x3F)));
}

int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool copyJsonStringValue(const char *firstQuote, char *out, size_t outSize) {
  if (!firstQuote || *firstQuote != '"' || !out || outSize == 0) {
    return false;
  }

  size_t pos = 0;
  bool truncated = false;
  const char *cursor = firstQuote + 1;
  auto appendDecoded = [&](char value) {
    if (pos + 1 >= outSize) {
      truncated = true;
      out[outSize - 1] = '\0';
      return true;
    }
    out[pos++] = value;
    out[pos] = '\0';
    return true;
  };

  while (*cursor && *cursor != '"') {
    if (*cursor == '\\') {
      cursor++;
      if (*cursor == '\0') {
        return false;
      }

      switch (*cursor) {
        case '"':
        case '\\':
        case '/':
          if (!appendDecoded(*cursor)) return false;
          cursor++;
          break;
        case 'n':
          if (!appendDecoded('\n')) return false;
          cursor++;
          break;
        case 'r':
          if (!appendDecoded('\r')) return false;
          cursor++;
          break;
        case 't':
          if (!appendDecoded('\t')) return false;
          cursor++;
          break;
        case 'u': {
          uint32_t codepoint = 0;
          for (int i = 1; i <= 4; ++i) {
            const int digit = hexValue(cursor[i]);
            if (digit < 0) return false;
            codepoint = (codepoint << 4) | (uint32_t)digit;
          }
          if (!truncated && !appendUtf8(out, outSize, pos, codepoint)) {
            truncated = true;
            out[outSize - 1] = '\0';
          }
          cursor += 5;
          break;
        }
        default:
          if (!appendDecoded(*cursor)) return false;
          cursor++;
          break;
      }
    } else {
      if (!appendDecoded(*cursor)) return false;
      cursor++;
    }
  }

  if (*cursor != '"') {
    return false;
  }
  out[pos] = '\0';
  return pos > 0;
}

bool copyNamedJsonString(const char *json, const char *name, char *out, size_t outSize) {
  if (!json || !name || !out || outSize == 0) {
    return false;
  }

  const char *key = std::strstr(json, name);
  if (!key) {
    return false;
  }

  const char *colon = std::strchr(key + std::strlen(name), ':');
  if (!colon) {
    return false;
  }

  const char *quote = colon + 1;
  while (*quote && std::isspace((unsigned char)*quote)) {
    quote++;
  }
  if (*quote != '"') {
    return false;
  }
  return copyJsonStringValue(quote, out, outSize);
}
}

XiaozhiCloudProvider parseXiaozhiCloudProvider(const char *provider) {
  if (!hasValue(provider) || equalsIgnoreCase(provider, "local")) {
    return XiaozhiCloudProvider::Local;
  }
  if (equalsIgnoreCase(provider, "anthropic") ||
      equalsIgnoreCase(provider, "claude")) {
    return XiaozhiCloudProvider::Anthropic;
  }
  if (equalsIgnoreCase(provider, "deepseek") ||
      equalsIgnoreCase(provider, "openai") ||
      equalsIgnoreCase(provider, "openai-compatible")) {
    return XiaozhiCloudProvider::OpenAiChat;
  }
  return XiaozhiCloudProvider::Unsupported;
}

bool xiaozhiCloudConfigReady(const char *provider, const char *endpoint,
                             const char *token, const char *model) {
  const XiaozhiCloudProvider parsed = parseXiaozhiCloudProvider(provider);
  return (parsed == XiaozhiCloudProvider::Anthropic ||
          parsed == XiaozhiCloudProvider::OpenAiChat) &&
         hasValue(endpoint) && hasValue(token) && hasValue(model);
}

const char *xiaozhiCloudPathForProvider(const char *provider) {
  switch (parseXiaozhiCloudProvider(provider)) {
    case XiaozhiCloudProvider::Anthropic:
      return "/v1/messages";
    case XiaozhiCloudProvider::OpenAiChat:
      return "/chat/completions";
    case XiaozhiCloudProvider::Local:
    case XiaozhiCloudProvider::Unsupported:
    default:
      return "";
  }
}

bool formatXiaozhiHomeStatus(char *out, size_t outSize,
                             float tempC, float humidity,
                             bool presence, bool securityArmed,
                             const char *weatherText, const char *timeText) {
  if (!out || outSize == 0) {
    return false;
  }

  const int written = std::snprintf(
      out, outSize,
      "Indoor %.1fC, humidity %.0f%%, presence %s, security %s, weather %s, time %s.",
      tempC, humidity,
      presence ? "occupied" : "empty",
      securityArmed ? "armed" : "off",
      hasValue(weatherText) ? weatherText : "unknown",
      hasValue(timeText) ? timeText : "unknown");
  return written > 0 && (size_t)written < outSize;
}

bool buildXiaozhiAnthropicRequestJson(char *out, size_t outSize,
                                      const char *model, const char *prompt,
                                      const char *homeStatus) {
  if (!out || outSize == 0 || !hasValue(model)) {
    return false;
  }

  size_t pos = 0;
  out[0] = '\0';
  return appendRaw(out, outSize, pos, "{\"model\":\"") &&
         appendJsonEscaped(out, outSize, pos, model) &&
         appendRaw(out, outSize, pos, "\",\"max_tokens\":80,\"temperature\":0.4,\"messages\":[{\"role\":\"user\",\"content\":\"You are XiaoZhi, a concise smart-home voice assistant. Reply in one short Chinese sentence. User request: ") &&
         appendJsonEscaped(out, outSize, pos, prompt ? prompt : "Home status request") &&
         appendRaw(out, outSize, pos, ". Home status: ") &&
         appendJsonEscaped(out, outSize, pos, homeStatus ? homeStatus : "unknown") &&
         appendRaw(out, outSize, pos, "\"}]}");
}

bool parseXiaozhiAnthropicReply(const char *json, char *out, size_t outSize) {
  return copyNamedJsonString(json, "\"text\"", out, outSize);
}

bool buildXiaozhiOpenAiChatRequestJson(char *out, size_t outSize,
                                       const char *model, const char *prompt,
                                       const char *homeStatus) {
  if (!out || outSize == 0 || !hasValue(model)) {
    return false;
  }

  size_t pos = 0;
  out[0] = '\0';
  return appendRaw(out, outSize, pos, "{\"model\":\"") &&
         appendJsonEscaped(out, outSize, pos, model) &&
         appendRaw(out, outSize, pos, "\",\"thinking\":{\"type\":\"disabled\"},\"max_tokens\":256,\"temperature\":0.2,\"messages\":[{\"role\":\"system\",\"content\":\"你是小智，只能输出一句简短中文最终回答，不要解释。\"},{\"role\":\"user\",\"content\":\"用户请求：") &&
         appendJsonEscaped(out, outSize, pos, prompt ? prompt : "Home status request") &&
         appendRaw(out, outSize, pos, "。家里状态：") &&
         appendJsonEscaped(out, outSize, pos, homeStatus ? homeStatus : "unknown") &&
         appendRaw(out, outSize, pos, "。请一句话回答。\"}]}");
}

bool parseXiaozhiOpenAiChatReply(const char *json, char *out, size_t outSize) {
  return copyNamedJsonString(json, "\"content\"", out, outSize);
}
