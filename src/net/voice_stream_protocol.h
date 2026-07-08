#pragma once

#include <cstddef>

enum class VoiceRelayMessageType {
  Ready,
  Phase,
  Asr,
  Reply,
  Done,
  Error,
};

struct VoiceRelayMessage {
  VoiceRelayMessageType type = VoiceRelayMessageType::Error;
  char text[192] = {};
};

bool parseVoiceRelayMessage(const char *json, VoiceRelayMessage &message);
