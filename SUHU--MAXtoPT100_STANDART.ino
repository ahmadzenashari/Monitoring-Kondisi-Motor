#include <Adafruit_MAX31865.h>

// =========================
// Konfigurasi MAX31865 (PT100)
// =========================
#define MAX31865_CS   10
#define MAX31865_MOSI 11
#define MAX31865_MISO 12
#define MAX31865_SCK  13

Adafruit_MAX31865 thermo = Adafruit_MAX31865(MAX31865_CS, MAX31865_MOSI, MAX31865_MISO, MAX31865_SCK);
#define SENSOR_TYPE MAX31865_3WIRE   // PT100 3-wire

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=====================================");
  Serial.println(" MAX31865 + PT100 Reader");
  Serial.println("=====================================");

  // Inisialisasi MAX31865
  if (!thermo.begin(SENSOR_TYPE)) {
    Serial.println("❌ Gagal inisialisasi MAX31865!");
    Serial.println("➡️  Periksa koneksi dan jumper 2/3-wire.");
    while (1);
  }
  Serial.println("✔ MAX31865 OK!");
}

void loop() {
  // ============================
  // Baca suhu PT100 via MAX31865
  // ============================
  float pt100_temp = thermo.temperature(100.0, 430.0);

  Serial.println(pt100_temp, 2);

  // Cek fault MAX31865
  uint8_t fault = thermo.readFault();
  if (fault) {
    Serial.print("⚠ Fault code: 0x");
    Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) Serial.println("  Suhu terlalu tinggi!");
    if (fault & MAX31865_FAULT_LOWTHRESH)  Serial.println("  Suhu terlalu rendah!");
    if (fault & MAX31865_FAULT_REFINLOW)   Serial.println("  REFIN- di bawah batas!");
    if (fault & MAX31865_FAULT_REFINHIGH)  Serial.println("  REFIN- di atas batas!");
    if (fault & MAX31865_FAULT_RTDINLOW)   Serial.println("  RTDIN- di bawah batas!");
    if (fault & MAX31865_FAULT_OVUV)       Serial.println("  Tegangan over/under!");

    thermo.clearFault();
  }

  delay(1000);
}
