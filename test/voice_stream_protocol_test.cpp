#include <cassert>
#include <cstring>

#include "../src/net/voice_stream_protocol.h"

int main() {
  VoiceRelayMessage message{};
  assert(parseVoiceRelayMessage("{\"type\":\"asr\",\"text\":\"ni hao\"}", message));
  assert(message.type == VoiceRelayMessageType::Asr);
  assert(std::strcmp(message.text, "ni hao") == 0);

  assert(parseVoiceRelayMessage("{\"type\":\"phase\",\"value\":\"thinking\"}", message));
  assert(message.type == VoiceRelayMessageType::Phase);
  assert(std::strcmp(message.text, "thinking") == 0);

  assert(parseVoiceRelayMessage("{\"type\":\"reply\",\"text\":\"hello\\\\world\"}", message));
  assert(std::strcmp(message.text, "hello\\world") == 0);

  assert(!parseVoiceRelayMessage("{\"type\":\"unknown\"}", message));
  assert(!parseVoiceRelayMessage("not-json", message));
}
