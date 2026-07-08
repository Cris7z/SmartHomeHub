#include "hardware.h"

#include <Arduino.h>
#include <driver/i2s.h>

#include "config.h"
#include "lamp_effect.h"
#include "speaker_tone.h"

Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_MOSI, PIN_TFT_SCK, PIN_TFT_RST);
Adafruit_AHTX0 aht;
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, PIN_STATUS_NEOPIXEL, NEO_GRB + NEO_KHZ800);

namespace {
Adafruit_NeoPixel onboardRgb(ONBOARD_RGB_COUNT, PIN_ONBOARD_RGB, NEO_GRB + NEO_KHZ800);
bool speakerInstalled = false;
bool speakerToneActive = false;
uint32_t speakerToneFrameIndex = 0;
uint32_t speakerToneTotalFrames = 0;
uint16_t speakerToneFrequency = SPEAKER_TONE_HZ;
int16_t speakerToneAmplitude = SPEAKER_TONE_AMPLITUDE;
bool speakerClipActive = false;
const int16_t *speakerClipSamples = nullptr;
size_t speakerClipSampleCount = 0;
size_t speakerClipFrameIndex = 0;
SpeakerToneFrame speakerToneBuffer[SPEAKER_TONE_CHUNK_FRAMES];
}

void clearOnboardRgb() {
  if (!shouldClearOnboardRgb(PIN_ONBOARD_RGB, PIN_STATUS_NEOPIXEL)) {
    return;
  }

  onboardRgb.begin();
  onboardRgb.clear();
  onboardRgb.show();
}

void setupOutputPins() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_TFT_BL, OUTPUT);
  pinMode(PIN_IR_TX, OUTPUT);

  buzzerWrite(false);
  digitalWrite(PIN_IR_TX, LOW);
  digitalWrite(PIN_TFT_BL, HIGH);
}

void setupStatusLamp() {
  clearOnboardRgb();
  strip.begin();
  strip.setBrightness(60);
  strip.clear();
  strip.show();
  clearOnboardRgb();
}

void buzzerWrite(bool on) {
  digitalWrite(PIN_BUZZER, on ? BUZZER_ACTIVE_LEVEL : !BUZZER_ACTIVE_LEVEL);
}

void setLamp(bool on, bool alarm) {
  const LampColor lampColor = lampColorFor(on, alarm, millis());
  const uint32_t color = strip.Color(lampColor.r, lampColor.g, lampColor.b);

  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void sendIrDemoBurst() {
  // Demo-only 38 kHz carrier burst. It proves the IR LED is driven, but it is not
  // a real air-conditioner protocol. Replace with a learned AC library/code later.
  for (int i = 0; i < IR_DEMO_BURST_CYCLES; i++) {
    digitalWrite(PIN_IR_TX, HIGH);
    delayMicroseconds(13);
    digitalWrite(PIN_IR_TX, LOW);
    delayMicroseconds(13);
  }
}

bool setupI2sMic() {
  i2s_config_t i2sConfig = {};
  i2sConfig.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  i2sConfig.sample_rate = MIC_SAMPLE_RATE;
  i2sConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  i2sConfig.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2sConfig.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2sConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2sConfig.dma_buf_count = 4;
  i2sConfig.dma_buf_len = 128;
  i2sConfig.use_apll = false;
  i2sConfig.tx_desc_auto_clear = false;
  i2sConfig.fixed_mclk = 0;

  i2s_pin_config_t pinConfig = {};
  pinConfig.bck_io_num = PIN_MIC_SCK;
  pinConfig.ws_io_num = PIN_MIC_WS;
  pinConfig.data_out_num = I2S_PIN_NO_CHANGE;
  pinConfig.data_in_num = PIN_MIC_SD;

  if (i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, nullptr) != ESP_OK) {
    return false;
  }
  if (i2s_set_pin(I2S_NUM_0, &pinConfig) != ESP_OK) {
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  return true;
}

bool setupI2sSpeaker() {
  i2s_config_t i2sConfig = {};
  i2sConfig.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2sConfig.sample_rate = SPEAKER_SAMPLE_RATE;
  i2sConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  i2sConfig.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2sConfig.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2sConfig.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2sConfig.dma_buf_count = 4;
  i2sConfig.dma_buf_len = 256;
  i2sConfig.use_apll = false;
  i2sConfig.tx_desc_auto_clear = true;
  i2sConfig.fixed_mclk = 0;

  i2s_pin_config_t pinConfig = {};
  pinConfig.bck_io_num = PIN_SPEAKER_BCLK;
  pinConfig.ws_io_num = PIN_SPEAKER_LRC;
  pinConfig.data_out_num = PIN_SPEAKER_DIN;
  pinConfig.data_in_num = I2S_PIN_NO_CHANGE;

  if (i2s_driver_install(I2S_NUM_1, &i2sConfig, 0, nullptr) != ESP_OK) {
    speakerInstalled = false;
    return false;
  }
  if (i2s_set_pin(I2S_NUM_1, &pinConfig) != ESP_OK) {
    i2s_driver_uninstall(I2S_NUM_1);
    speakerInstalled = false;
    return false;
  }
  i2s_zero_dma_buffer(I2S_NUM_1);
  speakerInstalled = true;
  return true;
}

bool queueSpeakerTone(uint16_t frequencyHz, uint16_t durationMs) {
  if (!speakerInstalled || frequencyHz == 0 || durationMs == 0) {
    return false;
  }

  speakerClipActive = false;
  speakerClipSamples = nullptr;
  speakerClipSampleCount = 0;
  speakerClipFrameIndex = 0;
  speakerToneFrequency = frequencyHz;
  speakerToneAmplitude = SPEAKER_TONE_AMPLITUDE;
  speakerToneFrameIndex = 0;
  speakerToneTotalFrames = (uint32_t)SPEAKER_SAMPLE_RATE * durationMs / 1000;
  speakerToneActive = speakerToneTotalFrames > 0;
  return speakerToneActive;
}

bool queueSpeakerPcmClip(const int16_t *samples, size_t sampleCount) {
  if (!speakerInstalled || !samples || sampleCount == 0) {
    return false;
  }

  speakerToneActive = false;
  speakerClipSamples = samples;
  speakerClipSampleCount = sampleCount;
  speakerClipFrameIndex = 0;
  speakerClipActive = true;
  return true;
}

void updateSpeakerAudio() {
  if (!speakerInstalled || (!speakerToneActive && !speakerClipActive)) {
    return;
  }

  size_t framesToWrite = 0;
  if (speakerClipActive) {
    const size_t framesRemaining = speakerClipSampleCount - speakerClipFrameIndex;
    framesToWrite = framesRemaining < SPEAKER_TONE_CHUNK_FRAMES
        ? framesRemaining
        : SPEAKER_TONE_CHUNK_FRAMES;
    framesToWrite = fillSpeakerPcmFrames(speakerToneBuffer, framesToWrite,
                                         speakerClipSamples, speakerClipSampleCount,
                                         speakerClipFrameIndex);
  } else {
    const uint32_t framesRemaining = speakerToneTotalFrames - speakerToneFrameIndex;
    framesToWrite = framesRemaining < SPEAKER_TONE_CHUNK_FRAMES
        ? (size_t)framesRemaining
        : SPEAKER_TONE_CHUNK_FRAMES;
    fillSpeakerToneFrames(speakerToneBuffer, framesToWrite, speakerToneFrameIndex,
                          SPEAKER_SAMPLE_RATE, speakerToneFrequency,
                          speakerToneAmplitude);
  }
  size_t bytesWritten = 0;
  if (framesToWrite == 0) {
    speakerToneActive = false;
    speakerClipActive = false;
    i2s_zero_dma_buffer(I2S_NUM_1);
    return;
  }

  const esp_err_t result = i2s_write(I2S_NUM_1, speakerToneBuffer,
                                    framesToWrite * sizeof(speakerToneBuffer[0]),
                                    &bytesWritten, pdMS_TO_TICKS(1));
  if (result != ESP_OK || bytesWritten == 0) {
    return;
  }

  const size_t framesWritten = bytesWritten / sizeof(speakerToneBuffer[0]);
  if (speakerClipActive) {
    speakerClipFrameIndex += framesWritten;
    if (speakerClipFrameIndex >= speakerClipSampleCount) {
      speakerClipActive = false;
      i2s_zero_dma_buffer(I2S_NUM_1);
    }
  } else {
    speakerToneFrameIndex += (uint32_t)framesWritten;
    if (speakerToneFrameIndex >= speakerToneTotalFrames) {
      speakerToneActive = false;
    }
  }
}

bool isSpeakerTonePlaying() {
  return speakerToneActive || speakerClipActive;
}
