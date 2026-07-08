#pragma once

#include <cstddef>
#include <cstdint>

bool setupVoiceCapture();
bool pollVoiceCapturePacket(int16_t *out, size_t outCapacity, size_t &sampleCount);
size_t copyVoicePreRoll(int16_t *out, size_t outCapacity);
void beginVoiceCaptureTurn();
void endVoiceCaptureTurn();
bool voiceCaptureOwnsMic();
