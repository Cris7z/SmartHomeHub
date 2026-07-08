#pragma once

#include <cstddef>
#include <cstdint>

bool setupStreamingSpeaker(size_t sampleCapacity);
bool beginStreamingSpeaker(uint32_t sampleRate);
size_t queueStreamingSpeakerPcm(const uint8_t *littleEndianBytes, size_t byteCount);
void updateStreamingSpeaker();
void endStreamingSpeaker();
bool streamingSpeakerActive();
bool streamingSpeakerOverflowed();

void cancelSpeakerLocalPlayback();
