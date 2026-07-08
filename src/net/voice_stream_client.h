#pragma once

void setupVoiceStreamClient();
void updateVoiceStreamClient();
bool startVoiceStreamTurn();
void cancelVoiceStreamTurn(const char *reason);
bool voiceStreamReady();
bool voiceStreamTurnActive();
