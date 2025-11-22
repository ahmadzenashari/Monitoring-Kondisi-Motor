#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MAX31865.h>
#include "driver/i2s.h"

// ====================== WIFI ======================
const char* ssid = "Gedung pusat";
const char* password = "0987654321";

// GANTI INI SESUAI URL DATABASE KAMU
const char* FIREBASE_HOST = "https://motor-monitoring-e52f3-default-rtdb.asia-southeast1.firebasedatabase.app";

// ====================== MPU6050 ======================
Adafruit_MPU6050 mpu;
float prevX = 0, prevY = 0, prevZ = 0;

// ====================== INMP441 ======================
#define I2S_WS   6   
#define I2S_SCK  5   
#define I2S_SD   4   
#define I2S_PORT I2S_NUM_0

void setupI2S() {
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_start(I2S_PORT);
}

// ====================== RPM ======================
#define RPM_PIN 7
volatile unsigned long pulseCount = 0;
unsigned long lastTime = 0;
float rpm = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

// ====================== MAX31865 (PT100) ======================
#define MAX31865_CS   10
#define MAX31865_MOSI 11
#define MAX31865_MISO 12
#define MAX31865_SCK  13
#define SENSOR_TYPE   MAX31865_3WIRE

Adafruit_MAX31865 thermo = Adafruit_MAX31865(MAX31865_CS, MAX31865_MOSI, MAX31865_MISO, MAX31865_SCK);

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\nBooting...\n");

  // ===== WIFI CONNECT =====
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  // ===== MPU6050 =====
  Wire.begin(8, 9);
  if (!mpu.begin()) {
    Serial.println("MPU6050 tidak terdeteksi!");
    while (1);
  }

  // ===== INMP441 =====
  setupI2S();

  // ===== RPM =====
  pinMode(RPM_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RPM_PIN), countPulse, RISING);

  // ===== MAX31865 =====
  if (!thermo.begin(SENSOR_TYPE)) {
    Serial.println("Gagal inisialisasi MAX31865!");
    while (1);
  }

  Serial.println("Setup selesai!\n");
}

// =============================== LOOP =================================
void loop() {

  // ===== Vibrasi =====
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float diffX = fabs(a.acceleration.x - prevX);
  float diffY = fabs(a.acceleration.y - prevY);
  float diffZ = fabs(a.acceleration.z - prevZ);
  float vibration = diffX + diffY + diffZ;

  prevX = a.acceleration.x;
  prevY = a.acceleration.y;
  prevZ = a.acceleration.z;

  // ===== Suara =====
  int32_t samples[128];
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, (void*)samples, sizeof(samples), &bytes_read, portMAX_DELAY);
  int sampleCount = bytes_read / sizeof(int32_t);
  long total = 0;
  for (int i = 0; i < sampleCount; i++) {
    total += abs(samples[i]);
  }
  float micLevel = sampleCount ? (float)total / sampleCount : 0;

  // ===== RPM =====
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {
    rpm = pulseCount * 60.0;
    pulseCount = 0;
    lastTime = currentTime;
  }

  // ===== Suhu PT100 =====
  float temperature = thermo.temperature(100.0, 430.0);

  // ===== KIRIM KE FIREBASE =====
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(FIREBASE_HOST) + "/sensor.json";
    http.begin(url);

    http.addHeader("Content-Type", "application/json");

    String jsonData = "{";
    jsonData += "\"vibrasi\":" + String(vibration, 3) + ",";
    jsonData += "\"suara\":" + String(micLevel, 0) + ",";
    jsonData += "\"rpm\":" + String(rpm, 0) + ",";
    jsonData += "\"suhu\":" + String(temperature, 2);
    jsonData += "}";

    int httpResponseCode = http.PUT(jsonData);

    Serial.print("Firebase Response: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0)
      Serial.println("Data terkirim!");
    else
      Serial.println("Gagal mengirim ke Firebase");

    http.end();
  }

  // ===== TAMPILKAN KE SERIAL =====
  Serial.print("Vibrasi: ");
  Serial.print(vibration, 3);
  Serial.print(" | Suara: ");
  Serial.print(micLevel, 0);
  Serial.print(" | RPM: ");
  Serial.print(rpm, 0);
  Serial.print(" | Suhu: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");

  delay(500);
}
