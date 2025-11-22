#include <Wire.h>

// ========================
// MPU6050
// ========================
#define MPU_ADDR 0x68

#define SDA_PIN 8
#define SCL_PIN 9

const float ACCEL_SENSITIVITY_MPU = 16384.0; // ±2g

float alpha = 0.98; // high pass untuk getaran
float hp_x_mpu = 0, hp_y_mpu = 0, hp_z_mpu = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  // ========================
  // INIT MPU6050
  // ========================
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);      // Wake up
  Wire.endTransmission();

  // Range ±2g
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission();

  Serial.println("\nMPU6050 READY (Vibration RMS)\n");
}

// ========================
// FUNGSI HIGH PASS
// ========================
float hpFilter(float val, float &lastVal, float &hp_out) {
  hp_out = alpha * (hp_out + val - lastVal);
  lastVal = val;
  return hp_out;
}

void loop() {

  // ========================
  // BACA MPU6050
  // ========================
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  int16_t ax_raw = (Wire.read() << 8) | Wire.read();
  int16_t ay_raw = (Wire.read() << 8) | Wire.read();
  int16_t az_raw = (Wire.read() << 8) | Wire.read();

  float ax_g = ax_raw / ACCEL_SENSITIVITY_MPU;
  float ay_g = ay_raw / ACCEL_SENSITIVITY_MPU;
  float az_g = az_raw / ACCEL_SENSITIVITY_MPU;

  static float lx_mpu = 0, ly_mpu = 0, lz_mpu = 0;

  hpFilter(ax_g, lx_mpu, hp_x_mpu);
  hpFilter(ay_g, ly_mpu, hp_y_mpu);
  hpFilter(az_g, lz_mpu, hp_z_mpu);

  float rms_mpu = sqrt(hp_x_mpu * hp_x_mpu +
                       hp_y_mpu * hp_y_mpu +
                       hp_z_mpu * hp_z_mpu);

  // ========================
  // OUTPUT
  // ========================
  Serial.print("MPU RMS: ");
  Serial.print(rms_mpu, 4);
  Serial.println(" g");

  delay(50);
}
