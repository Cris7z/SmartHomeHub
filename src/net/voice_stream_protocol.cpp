#include "voice_stream_protocol.h"

#include <cstring>

namespace {

constexpr size_t TEXT_CAPACITY = sizeof(VoiceRelayMessage::text);

const char *findKeyValueStart(const char *json, const char *key) {
  const char *pos = std::strstr(json, key);
  if (!pos) {
    return nullptr;
  }
  pos += std::strlen(key);
  while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
    ++pos;
  }
  if (*pos != ':') {
    return nullptr;
  }
  ++pos;
  while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
    ++pos;
  }
  return *pos == '"' ? pos + 1 : nullptr;
}

bool readJsonString(const char *start, char *out, size_t outCapacity) {
  if (!start || !out || outCapacity == 0) {
    return false;
  }

  size_t written = 0;
  for (const char *p = start; *p; ++p) {
    char value = *p;
    if (value == '"') {
      out[written] = '\0';
      return true;
    }
    if (value == '\\') {
      ++p;
      if (!*p) {
        return false;
      }
      switch (*p) {
        case '"':
        case '\\':
        case '/':
          value = *p;
          break;
        case 'n':
          value = '\n';
          break;
        case 'r':
          value = '\r';
          break;
        case 't':
          value = '\t';
          break;
        default:
          return false;
      }
    }
    if (written + 1 >= outCapacity) {
      return false;
    }
    out[written++] = value;
  }
  return false;
}

bool readField(const char *json, const char *key, char *out, size_t outCapacity) {
  return readJsonString(findKeyValueStart(json, key), out, outCapacity);
}

bool mapType(const char *type, VoiceRelayMessageType &mapped) {
  if (std::strcmp(type, "ready") == 0) {
    mapped = VoiceRelayMessageType::Ready;
  } else if (std::strcmp(type, "phase") == 0) {
    mapped = VoiceRelayMessageType::Phase;
  } else if (std::strcmp(type, "asr") == 0) {
    mapped = VoiceRelayMessageType::Asr;
  } else if (std::strcmp(type, "reply") == 0) {
    mapped = VoiceRelayMessageType::Reply;
  } else if (std::strcmp(type, "action") == 0) {
    mapped = VoiceRelayMessageType::Action;
  } else if (std::strcmp(type, "done") == 0) {
    mapped = VoiceRelayMessageType::Done;
  } else if (std::strcmp(type, "error") == 0) {
    mapped = VoiceRelayMessageType::Error;
  } else {
    return false;
  }
  return true;
}

}  // namespace

bool parseVoiceRelayMessage(const char *json, VoiceRelayMessage &message) {
  if (!json || json[0] != '{') {
    return false;
  }

  char type[16] = {};
  if (!readField(json, "\"type\"", type, sizeof(type))) {
    return false;
  }

  VoiceRelayMessage parsed{};
  if (!mapType(type, parsed.type)) {
    return false;
  }

  if (parsed.type == VoiceRelayMessageType::Phase) {
    if (!readField(json, "\"value\"", parsed.text, TEXT_CAPACITY)) {
      return false;
    }
  } else if (parsed.type == VoiceRelayMessageType::Asr ||
             parsed.type == VoiceRelayMessageType::Reply) {
    if (!readField(json, "\"text\"", parsed.text, TEXT_CAPACITY)) {
      return false;
    }
  } else if (parsed.type == VoiceRelayMessageType::Action) {
    if (!readField(json, "\"name\"", parsed.name, sizeof(parsed.name))) {
      return false;
    }
    readField(json, "\"text\"", parsed.text, TEXT_CAPACITY);
  } else if (parsed.type == VoiceRelayMessageType::Error) {
    if (!readField(json, "\"message\"", parsed.text, TEXT_CAPACITY)) {
      readField(json, "\"code\"", parsed.text, TEXT_CAPACITY);
    }
  }

  message = parsed;
  return true;
}
