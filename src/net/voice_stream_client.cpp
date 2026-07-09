#include "voice_stream_client.h"

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <cstring>

#include "../app/controls.h"
#include "../app/hub_state.h"
#include "../app/xiaozhi_core.h"
#include "../board/config.h"
#include "../board/hardware.h"
#include "../io/voice_capture.h"
#include "secrets_config.h"
#include "voice_stream_protocol.h"

namespace {
WebSocketsClient voiceWs;
String relayHost;
String relayPath = "/voice";
uint16_t relayPort = 8765;
bool relayConfigured = false;
bool relayConnected = false;
bool helloSent = false;
bool turnActive = false;
bool relaySessionListening = false;
bool endAudioSent = false;
bool preRollSent = false;
uint32_t turnStartedMs = 0;
uint32_t lastVoiceActivityMs = 0;
int16_t voicePacket[VOICE_PACKET_SAMPLES] = {};
int16_t voicePreRoll[VOICE_PREROLL_SAMPLES] = {};
char relayTextBuffer[256] = {};

void copyBounded(char *dest, size_t capacity, const char *src) {
  if (!dest || capacity == 0) {
    return;
  }
  if (!src) {
    dest[0] = '\0';
    return;
  }
  std::strncpy(dest, src, capacity - 1);
  dest[capacity - 1] = '\0';
}

bool parseRelayUrl(const char *url) {
  String value(url ? url : "");
  value.trim();
  if (!value.startsWith("ws://")) {
    return false;
  }
  value.remove(0, 5);
  const int slash = value.indexOf('/');
  const String hostPort = slash >= 0 ? value.substring(0, slash) : value;
  relayPath = slash >= 0 ? value.substring(slash) : "/voice";
  const int colon = hostPort.lastIndexOf(':');
  if (colon >= 0) {
    relayHost = hostPort.substring(0, colon);
    relayPort = (uint16_t)hostPort.substring(colon + 1).toInt();
  } else {
    relayHost = hostPort;
    relayPort = 80;
  }
  return relayHost.length() > 0 && relayPort > 0 && relayPath.length() > 0;
}

String startTurnJson() {
  String body = "{\"type\":\"start_turn\",\"protocol\":1,\"home\":{\"temp_c\":";
  body += String(state.tempC, 1);
  body += ",\"humidity\":";
  body += String(state.humidity, 0);
  body += ",\"presence\":";
  body += state.presence ? "true" : "false";
  body += ",\"security_armed\":";
  body += state.securityArmed ? "true" : "false";
  body += ",\"force_security\":";
  body += state.forceSecurity ? "true" : "false";
  body += ",\"alarm\":";
  body += state.alarm ? "true" : "false";
  body += ",\"sound\":";
  body += state.soundTriggered ? "true" : "false";
  body += ",\"risk_score\":";
  body += String(state.aiRiskScore);
  body += ",\"risk\":\"";
  body += state.aiRiskText;
  body += "\",\"lamp_on\":";
  body += (state.alarm || (state.lampOverride ? state.manualLamp : state.presence)) ? "true" : "false";
  body += ",\"lamp_override\":";
  body += state.lampOverride ? "true" : "false";
  body += ",\"ac_cooling\":";
  body += state.acCooling ? "true" : "false";
  body += ",\"ir_rx\":";
  body += state.irReceived ? "true" : "false";
  body += ",\"ir_test\":";
  body += state.irTestActive ? "true" : "false";
  body += ",\"time\":\"";
  body += state.timeReady ? state.timeText : "--:--";
  body += "\",\"weather\":\"";
  body += state.weatherReady ? state.weatherText : "--";
  body += "\",\"location\":\"";
  body += state.locationText;
  body += "\"";
  body += "}}";
  return body;
}

void applyRelayAction(const char *name, const char *text) {
  if (!name || !name[0]) {
    return;
  }

  if (std::strcmp(name, "security_on") == 0) {
    applyHubCommand(HubCommand::SetSecurityOn, "XIAOZHI");
  } else if (std::strcmp(name, "security_off") == 0) {
    applyHubCommand(HubCommand::SetSecurityOff, "XIAOZHI");
  } else if (std::strcmp(name, "lamp_on") == 0) {
    applyHubCommand(HubCommand::SetLampOn, "XIAOZHI");
  } else if (std::strcmp(name, "lamp_off") == 0) {
    applyHubCommand(HubCommand::SetLampOff, "XIAOZHI");
  } else if (std::strcmp(name, "ac_on") == 0) {
    applyHubCommand(HubCommand::SetAcOn, "XIAOZHI");
  } else if (std::strcmp(name, "ac_off") == 0) {
    applyHubCommand(HubCommand::SetAcOff, "XIAOZHI");
  } else if (std::strcmp(name, "clear_alarm") == 0) {
    applyHubCommand(HubCommand::ClearAlarmSecurity, "XIAOZHI");
  } else if (std::strcmp(name, "macro_home") == 0) {
    applyHubCommand(HubCommand::RunMacroHome, "XIAOZHI");
  } else if (std::strcmp(name, "macro_away") == 0) {
    applyHubCommand(HubCommand::RunMacroAway, "XIAOZHI");
  } else if (std::strcmp(name, "macro_night") == 0) {
    applyHubCommand(HubCommand::RunMacroNight, "XIAOZHI");
  } else {
    return;
  }

  if (text && text[0]) {
    copyBounded(state.xiaozhiReplyText, sizeof(state.xiaozhiReplyText), text);
  }
  Serial.printf("[XIAOZHI] action=%s\n", name);
}

void sendHello() {
  if (relayConnected && !helloSent) {
    voiceWs.sendTXT("{\"type\":\"hello\",\"protocol\":1}");
    helloSent = true;
  }
}

void handleRelayText(const char *text) {
  VoiceRelayMessage message{};
  if (!parseVoiceRelayMessage(text, message)) {
    return;
  }
  switch (message.type) {
    case VoiceRelayMessageType::Ready:
      state.doubaoRelayConnected = true;
      copyBounded(state.xiaozhiStatusText, sizeof(state.xiaozhiStatusText), "READY");
      break;
    case VoiceRelayMessageType::Phase:
      copyBounded(state.xiaozhiStatusText, sizeof(state.xiaozhiStatusText), message.text);
      if (std::strcmp(message.text, "listening") == 0) {
        relaySessionListening = true;
        turnStartedMs = millis();
        lastVoiceActivityMs = turnStartedMs;
        state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::TurnStarted);
      } else if (std::strcmp(message.text, "thinking") == 0) {
        relaySessionListening = false;
        endAudioSent = true;
        state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AsrEnded);
      } else if (std::strcmp(message.text, "speaking") == 0) {
        relaySessionListening = false;
        endAudioSent = true;
        state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AudioStarted);
      }
      break;
    case VoiceRelayMessageType::Asr:
      copyBounded(state.xiaozhiPromptText, sizeof(state.xiaozhiPromptText), message.text);
      state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AsrEnded);
      break;
    case VoiceRelayMessageType::Reply:
      copyBounded(state.xiaozhiReplyText, sizeof(state.xiaozhiReplyText), message.text);
      break;
    case VoiceRelayMessageType::Action:
      applyRelayAction(message.name, message.text);
      break;
    case VoiceRelayMessageType::Done:
      turnActive = false;
      relaySessionListening = false;
      endAudioSent = false;
      state.doubaoSessionActive = false;
      state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::TurnDone);
      endVoiceCaptureTurn();
      finishStreamingSpeaker();
      copyBounded(state.xiaozhiStatusText, sizeof(state.xiaozhiStatusText), "DONE");
      break;
    case VoiceRelayMessageType::Error:
      turnActive = false;
      relaySessionListening = false;
      endAudioSent = false;
      state.doubaoSessionActive = false;
      state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::Failed);
      endVoiceCaptureTurn();
      endStreamingSpeaker();
      copyBounded(state.xiaozhiStatusText, sizeof(state.xiaozhiStatusText), "ERROR");
      copyBounded(state.xiaozhiReplyText, sizeof(state.xiaozhiReplyText), message.text);
      copyBounded(state.xiaozhiErrorText, sizeof(state.xiaozhiErrorText), message.text);
      break;
  }
}

void onVoiceWsEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      relayConnected = true;
      state.doubaoRelayConnected = true;
      helloSent = false;
      sendHello();
      break;
    case WStype_DISCONNECTED:
      relayConnected = false;
      state.doubaoRelayConnected = false;
      helloSent = false;
      turnActive = false;
      relaySessionListening = false;
      endAudioSent = false;
      state.doubaoSessionActive = false;
      endVoiceCaptureTurn();
      endStreamingSpeaker();
      break;
    case WStype_TEXT:
      if (payload) {
        const size_t copied = length < sizeof(relayTextBuffer) - 1
            ? length
            : sizeof(relayTextBuffer) - 1;
        std::memcpy(relayTextBuffer, payload, copied);
        relayTextBuffer[copied] = '\0';
        handleRelayText(relayTextBuffer);
      }
      break;
    case WStype_BIN:
      if (payload && length > 0) {
        if (!streamingSpeakerActive()) {
          state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AudioStarted);
          beginStreamingSpeaker(VOICE_OUTPUT_SAMPLE_RATE);
        }
        queueStreamingSpeakerPcm(payload, length);
      }
      break;
    default:
      break;
  }
}

void sendEndAudioIfDue(uint32_t nowMs) {
  if (!turnActive || !relayConnected || !relaySessionListening || endAudioSent) {
    return;
  }
  const bool heardEnough = nowMs - turnStartedMs >= VOICE_TURN_MIN_MS;
  const bool quietEnough = heardEnough && nowMs - lastVoiceActivityMs >= VOICE_TURN_SILENCE_MS;
  const bool hitMax = nowMs - turnStartedMs >= VOICE_TURN_MAX_MS;
  if (!quietEnough && !hitMax) {
    return;
  }

  voiceWs.sendTXT("{\"type\":\"end_audio\",\"protocol\":1}");
  endAudioSent = true;
  relaySessionListening = false;
  endVoiceCaptureTurn();
  copyBounded(state.xiaozhiStatusText, sizeof(state.xiaozhiStatusText), "thinking");
  state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AsrEnded);
}
}

void setupVoiceStreamClient() {
  relayConfigured = parseRelayUrl(SMART_HOME_DOUBAO_RELAY_URL);
  state.doubaoRelayConfigured = relayConfigured;
  setupVoiceCapture();
  if (!relayConfigured) {
    return;
  }
  voiceWs.begin(relayHost.c_str(), relayPort, relayPath.c_str());
  voiceWs.onEvent(onVoiceWsEvent);
  voiceWs.setReconnectInterval(5000);
}

void updateVoiceStreamClient() {
  if (relayConfigured) {
    voiceWs.loop();
    sendHello();
  }

  size_t sampleCount = 0;
  const bool captured = pollVoiceCapturePacket(voicePacket, VOICE_PACKET_SAMPLES, sampleCount);
  const uint32_t now = millis();
  if (turnActive && relaySessionListening && state.soundTriggered) {
    lastVoiceActivityMs = now;
  }
  sendEndAudioIfDue(now);
  if (!turnActive || !relayConnected || !relaySessionListening || endAudioSent || !captured) {
    return;
  }
  if (!preRollSent) {
    const size_t preRollSamples = copyVoicePreRoll(voicePreRoll, VOICE_PREROLL_SAMPLES);
    if (preRollSamples > 0) {
      voiceWs.sendBIN(reinterpret_cast<uint8_t *>(voicePreRoll),
                      preRollSamples * sizeof(voicePreRoll[0]));
    }
    preRollSent = true;
  }
  if (sampleCount == VOICE_PACKET_SAMPLES) {
    voiceWs.sendBIN(reinterpret_cast<uint8_t *>(voicePacket), sampleCount * sizeof(voicePacket[0]));
  }
}

bool startVoiceStreamTurn() {
  if (!relayConfigured || !relayConnected || turnActive) {
    return false;
  }
  beginVoiceCaptureTurn();
  turnActive = true;
  relaySessionListening = false;
  endAudioSent = false;
  state.doubaoSessionActive = true;
  state.xiaozhiPhase = (uint8_t)xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::TurnStarted);
  turnStartedMs = millis();
  lastVoiceActivityMs = turnStartedMs;
  preRollSent = false;
  String body = startTurnJson();
  voiceWs.sendTXT(body);
  return true;
}

void cancelVoiceStreamTurn(const char *reason) {
  if (turnActive && relayConnected) {
    String body = "{\"type\":\"cancel\",\"protocol\":1}";
    voiceWs.sendTXT(body);
  }
  (void)reason;
  turnActive = false;
  relaySessionListening = false;
  endAudioSent = false;
  state.doubaoSessionActive = false;
  endVoiceCaptureTurn();
  endStreamingSpeaker();
}

bool voiceStreamReady() {
  return relayConfigured && relayConnected;
}

bool voiceStreamTurnActive() {
  return turnActive;
}

bool voiceStreamConfigured() {
  return relayConfigured;
}
