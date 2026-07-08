#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

std::string readFile(const char *path) {
  std::ifstream file(path);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

int main() {
  const std::string ai = readFile("src/app/xiaozhi_ai.cpp");
  const std::string mainSource = readFile("src/main.cpp");
  const std::string dashboard = readFile("src/net/web_dashboard.cpp");
  const std::string secrets = readFile("src/net/secrets_config.h");

  assert(ai.find("startVoiceStreamTurn") != std::string::npos);
  assert(ai.find("voiceStreamTurnActive") != std::string::npos);
  assert(mainSource.find("updateVoiceStreamClient") != std::string::npos);
  assert(mainSource.find("updateStreamingSpeaker") != std::string::npos);
  assert(dashboard.find("speechSynthesis.speak") == std::string::npos);
  assert(secrets.find("SMART_HOME_DOUBAO_RELAY_URL") != std::string::npos);

  const std::string voiceClient = readFile("src/net/voice_stream_client.cpp");
  const std::string voiceCapture = readFile("src/io/voice_capture.cpp");
  const std::string streamingSpeaker = readFile("src/board/streaming_speaker.cpp");
  const std::string streamingSpeakerHeader = readFile("src/board/streaming_speaker.h");
  const std::string config = readFile("src/board/config.h");
  const std::string hardware = readFile("src/board/hardware.cpp");
  assert(voiceClient.find("relaySessionListening = true") != std::string::npos);
  assert(voiceClient.find("!relaySessionListening") != std::string::npos);
  assert(voiceClient.find("end_audio") != std::string::npos);
  assert(voiceClient.find("VOICE_TURN_MAX_MS") != std::string::npos);
  assert(voiceClient.find("turnStartedMs = millis();") != std::string::npos);
  assert(voiceClient.find("finishStreamingSpeaker") != std::string::npos);
  assert(voiceCapture.find("analyzeI2sMicSamples(raw, (int)rawCount, MIC_SAMPLE_SHIFT)") != std::string::npos);
  assert(voiceCapture.find("VOICE_PCM_RIGHT_SHIFT, converted") != std::string::npos);
  assert(streamingSpeakerHeader.find("finishStreamingSpeaker") != std::string::npos);
  assert(streamingSpeaker.find("streamingFinishing") != std::string::npos);
  assert(streamingSpeaker.find("VOICE_OUTPUT_GAIN_PERCENT") != std::string::npos);
  assert(streamingSpeaker.find("STREAMING_SPEAKER_WRITE_BUDGET_FRAMES") != std::string::npos);
  assert(streamingSpeaker.find("STREAMING_SPEAKER_DRAIN_MS") != std::string::npos);
  assert(config.find("constexpr int VOICE_OUTPUT_GAIN_PERCENT = 240;") != std::string::npos);
  assert(config.find("constexpr size_t VOICE_OUTPUT_BUFFER_SECONDS = 8;") != std::string::npos);
  assert(hardware.find("i2sConfig.dma_buf_count = 8;") != std::string::npos);
  assert(hardware.find("i2sConfig.dma_buf_len = 512;") != std::string::npos);
  assert(hardware.find("VOICE_OUTPUT_SAMPLE_RATE * VOICE_OUTPUT_BUFFER_SECONDS") != std::string::npos);
}
