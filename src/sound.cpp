
#include <Arduino.h>
#include <M5Core2.h>
#include <driver/i2s.h>

#define LOAD_SOUND 0
#include "sound.h"

bool init_i2s(bool mic) {
  esp_err_t err = ESP_OK;
  i2s_driver_uninstall(I2S_NUM_0);
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER |
                           (mic ? I2S_MODE_RX | I2S_MODE_PDM : I2S_MODE_TX)),
      .sample_rate = 44100,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 2,
      .dma_buf_len = 128,
  };
  err += i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_pin_config_t tx_pin_config = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = CONFIG_I2S_BCK_PIN,
      .ws_io_num = CONFIG_I2S_LRCK_PIN,
      .data_out_num = CONFIG_I2S_DATA_PIN,
      .data_in_num = CONFIG_I2S_DATA_IN_PIN,
  };
  err += i2s_set_pin(I2S_NUM_0, &tx_pin_config);
  if (mic)
    err += i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT,
                       I2S_CHANNEL_MONO);
  i2s_zero_dma_buffer(I2S_NUM_0);

  M5.Axp.SetSpkEnable(!mic);
  return err == ESP_OK;
}

bool record_audio(void* buffer, size_t bufsize) {
  esp_err_t err = ESP_OK;
  if (bufsize % 128) return false; /* buffer size must be 128 x n [bytes] */
  if (!init_i2s(true)) return false;
  memset(buffer, 0, bufsize);

  size_t bytes_read;
  for (int i = 0; i < bufsize / 128; i++) {
    err = i2s_read(I2S_NUM_0, (uint8_t*)buffer + i * 128, 128, &bytes_read,
                   100 / portTICK_RATE_MS);
    if (err != ESP_OK) return false;
  }
  return err == ESP_OK;
}

bool play_audio(const void* buffer, size_t bufsize) {
  if (!init_i2s(false)) return false;
  size_t bytes_written;
  esp_err_t err =
      i2s_write(I2S_NUM_0, buffer, bufsize, &bytes_written, portMAX_DELAY);
  M5.Axp.SetSpkEnable(false);
  return err == ESP_OK;
}

void amplify(int16_t* buffer, size_t bufsize) {
  for (int i = 0; i < bufsize / 2; i++) {
    buffer[i] <<= 2;
  }
}