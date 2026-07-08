#pragma once

#include <Adafruit_AHTX0.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_NeoPixel.h>

extern Adafruit_ILI9341 tft;
extern Adafruit_AHTX0 aht;
extern Adafruit_NeoPixel strip;

void setupOutputPins();
void clearOnboardRgb();
void setupStatusLamp();
void buzzerWrite(bool on);
void setLamp(bool on, bool alarm);
void sendIrDemoBurst();
bool setupI2sMic();
bool setupI2sSpeaker();
bool queueSpeakerTone(uint16_t frequencyHz, uint16_t durationMs);
bool queueSpeakerPcmClip(const int16_t *samples, size_t sampleCount);
void updateSpeakerAudio();
bool isSpeakerTonePlaying();
