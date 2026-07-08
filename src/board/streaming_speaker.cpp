#include "streaming_speaker.h"

#include <Arduino.h>
#include <driver/i2s.h>
#include <esp_heap_caps.h>

#include "config.h"
#include "pcm_ring_buffer.h"
#include "speaker_tone.h"

namespace {
int16_t *streamingStorage = nullptr;
size_t streamingCapacity = 0;
PcmRingBuffer streamingQueue;
bool streamingActive = false;
bool streamingFinishing = false;
uint32_t lastStreamingWriteMs = 0;
SpeakerToneFrame streamingFrames[SPEAKER_TONE_CHUNK_FRAMES];
int16_t streamingMono[SPEAKER_TONE_CHUNK_FRAMES];

int16_t scaleVoiceSample(int16_t sample) {
  int32_t scaled = (int32_t)sample * VOICE_OUTPUT_GAIN_PERCENT / 100;
  if (scaled > INT16_MAX) {
    return INT16_MAX;
  }
  if (scaled < INT16_MIN) {
    return INT16_MIN;
  }
  return (int16_t)scaled;
}
}

bool setupStreamingSpeaker(size_t sampleCapacity) {
  if (sampleCapacity == 0) {
    return false;
  }
  if (streamingStorage) {
    heap_caps_free(streamingStorage);
    streamingStorage = nullptr;
  }

  streamingStorage = static_cast<int16_t *>(heap_caps_malloc(
      sampleCapacity * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!streamingStorage) {
    streamingStorage =
        static_cast<int16_t *>(heap_caps_malloc(sampleCapacity * sizeof(int16_t), MALLOC_CAP_8BIT));
  }
  if (!streamingStorage) {
    streamingCapacity = 0;
    streamingQueue = PcmRingBuffer();
    return false;
  }

  streamingCapacity = sampleCapacity;
  streamingQueue = PcmRingBuffer(streamingStorage, streamingCapacity);
  return true;
}

bool beginStreamingSpeaker(uint32_t sampleRate) {
  if (!streamingStorage || streamingCapacity == 0 || sampleRate == 0) {
    return false;
  }

  cancelSpeakerLocalPlayback();
  streamingQueue.clear();
  if (i2s_set_clk(I2S_NUM_1, sampleRate, I2S_BITS_PER_SAMPLE_16BIT,
                  I2S_CHANNEL_STEREO) != ESP_OK) {
    streamingActive = false;
    return false;
  }
  i2s_zero_dma_buffer(I2S_NUM_1);
  streamingActive = true;
  streamingFinishing = false;
  lastStreamingWriteMs = millis();
  return true;
}

size_t queueStreamingSpeakerPcm(const uint8_t *littleEndianBytes, size_t byteCount) {
  if (!streamingActive || !littleEndianBytes || byteCount < 2) {
    return 0;
  }

  const size_t sampleCount = byteCount / sizeof(int16_t);
  size_t accepted = 0;
  for (size_t i = 0; i < sampleCount; ++i) {
    const int16_t sample = static_cast<int16_t>(
        littleEndianBytes[i * 2] | (static_cast<uint16_t>(littleEndianBytes[i * 2 + 1]) << 8));
    if (streamingQueue.push(&sample, 1) != 1) {
      break;
    }
    ++accepted;
  }
  return accepted * sizeof(int16_t);
}

void updateStreamingSpeaker() {
  if (!streamingActive) {
    return;
  }
  if (streamingQueue.empty()) {
    if (streamingFinishing && millis() - lastStreamingWriteMs >= STREAMING_SPEAKER_DRAIN_MS) {
      endStreamingSpeaker();
    }
    return;
  }

  size_t framesQueued = 0;
  while (framesQueued < STREAMING_SPEAKER_WRITE_BUDGET_FRAMES && !streamingQueue.empty()) {
    const size_t samples = streamingQueue.pop(streamingMono, SPEAKER_TONE_CHUNK_FRAMES);
    if (samples == 0) {
      break;
    }
    for (size_t i = 0; i < samples; ++i) {
      const int16_t sample = scaleVoiceSample(streamingMono[i]);
      streamingFrames[i].left = sample;
      streamingFrames[i].right = sample;
    }

    size_t bytesWritten = 0;
    i2s_write(I2S_NUM_1, streamingFrames, samples * sizeof(streamingFrames[0]),
              &bytesWritten, pdMS_TO_TICKS(2));
    if (bytesWritten == 0) {
      break;
    }
    framesQueued += samples;
    lastStreamingWriteMs = millis();
  }
}

void finishStreamingSpeaker() {
  if (streamingActive) {
    streamingFinishing = true;
  }
}

void endStreamingSpeaker() {
  if (!streamingActive) {
    streamingFinishing = false;
    return;
  }

  streamingActive = false;
  streamingFinishing = false;
  lastStreamingWriteMs = 0;
  streamingQueue.clear();
  i2s_set_clk(I2S_NUM_1, SPEAKER_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
  i2s_zero_dma_buffer(I2S_NUM_1);
}

bool streamingSpeakerActive() {
  return streamingActive;
}

bool streamingSpeakerOverflowed() {
  return streamingQueue.overflowed();
}
