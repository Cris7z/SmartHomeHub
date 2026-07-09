#pragma once

#include <cstddef>

enum class VoiceRelayMessageType {
  Ready,
  Phase,
  Asr,
  Reply,
  Action,
  Done,
  Error,
};

struct VoiceRelayMessage {
  VoiceRelayMessageType type = VoiceRelayMessageType::Error;
  char name[32] = {};
  char text[192] = {};
};

bool parseVoiceRelayMessage(const char *json, VoiceRelayMessage &message);
