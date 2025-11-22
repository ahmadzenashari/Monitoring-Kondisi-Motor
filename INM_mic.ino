#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_WS   6   // L/R clock (WS)
#define I2S_SCK  5   // Bit clock (BCLK)
#define I2S_SD   4   // Data in (DOUT)

// ------------------------------
// Konfigurasi Audio
// ------------------------------
#define SAMPLE_RATE   16000
#define BUFFER_SIZE   1024

// ------------------------------
// Offset kalibrasi untuk SPL
// Lakukan kalibrasi dengan Sound Level Meter
// contoh umum: offset = 105
// ------------------------------
float SPL_OFFSET = 105.0;

void setup() {
  Serial.begin(115200);
  delay(500);

  // I2S config
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, 
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false
  };

  // Pin map
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  // Install driver
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println("INMP441 READY (dBFS + dB SPL)...");
}

void loop() {
  int32_t samples[BUFFER_SIZE];
  size_t bytes_read;

  // Baca data
  i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytes_read, portMAX_DELAY);

  int count = bytes_read / sizeof(int32_t);

  // ------------------------------
  // Hitung RMS
  // ------------------------------
  double sum_sq = 0;

  for (int i = 0; i < count; i++) {
    int32_t s = samples[i] >> 8;   // INMP441: 24-bit → shift ke 16-bit
    sum_sq += (double)s * (double)s;
  }

  double rms = sqrt(sum_sq / count);

  // ------------------------------
  // Konversi ke dBFS
  // ------------------------------
  double dbfs = 20.0 * log10(rms / 32768.0);

  // ------------------------------
  // Hitung dB SPL setelah kalibrasi
  // ------------------------------
  double db_spl = dbfs + SPL_OFFSET;

  // ------------------------------
  // Print hasil
  // ------------------------------
  Serial.print("RMS: ");
  Serial.print(rms);
  Serial.print(" | dBFS: ");
  Serial.print(dbfs);
  Serial.print(" | dB SPL: ");
  Serial.println(db_spl);

  delay(100);
}
