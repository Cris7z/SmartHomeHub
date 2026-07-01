#include "hardware.h"

#include <Arduino.h>
#include <driver/i2s.h>

#include "config.h"

Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_MOSI, PIN_TFT_SCK, PIN_TFT_RST);
Adafruit_AHTX0 aht;
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, PIN_STATUS_NEOPIXEL, NEO_GRB + NEO_KHZ800);

void setupOutputPins() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_TFT_BL, OUTPUT);
  pinMode(PIN_IR_TX, OUTPUT);

  buzzerWrite(false);
  digitalWrite(PIN_IR_TX, LOW);
  digitalWrite(PIN_TFT_BL, HIGH);
}

void setupStatusLamp() {
  strip.begin();
  strip.setBrightness(60);
  strip.show();
}

void buzzerWrite(bool on) {
  digitalWrite(PIN_BUZZER, on ? BUZZER_ACTIVE_LEVEL : !BUZZER_ACTIVE_LEVEL);
}

void setLamp(bool on, bool alarm) {
  uint32_t color = strip.Color(0, 0, 0);
  if (alarm) {
    color = strip.Color(255, 0, 0);
  } else if (on) {
    color = strip.Color(255, 180, 80);
  }

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
