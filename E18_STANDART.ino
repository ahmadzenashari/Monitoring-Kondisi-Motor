#define E18_PIN 7

// === Adaptive debounce ===
unsigned long lastPulseUS = 0;
unsigned long minDebounceUS = 4000;   // start 4 ms (akan berubah otomatis)

// === RPM calculation ===
unsigned long pulseCount = 0;
unsigned long lastRPMms = 0;
const unsigned long RPM_INTERVAL = 150;  // super cepat (150 ms)

// === Smoothing (moving average ringan) ===
float rpmFiltered = 0;
float alpha = 0.35;   // smoothing factor (0.2 = halus, 0.5 = cepat)

bool lastState = HIGH;

void setup() {
  Serial.begin(115200);
  pinMode(E18_PIN, INPUT_PULLUP);
}

void loop() {

  unsigned long nowUS = micros();
  bool state = digitalRead(E18_PIN);

  // ======== EDGE DETECTION SUPER CEPAT ========
  if (state != lastState) {

    // FALLING EDGE ONLY → pulse valid
    if (state == LOW) {

      unsigned long diff = nowUS - lastPulseUS;

      // Adaptive debounce:
      // Jika pulse datang cepat → motor sedang cepat → debounce diperkecil
      // Jika pulse lambat → motor lambat → debounce diperbesar
      if (diff > minDebounceUS) {
        pulseCount++;

        // Update adaptive debounce:
        // 20% dari jarak pulse terakhir
        minDebounceUS = diff * 0.20;
        if (minDebounceUS < 3000) minDebounceUS = 3000;   // batas bawah
        if (minDebounceUS > 20000) minDebounceUS = 20000; // batas atas

        lastPulseUS = nowUS;
      }
    }
    lastState = state;
  }

  // ======== PERHITUNGAN RPM SUPER CEPAT ========
  unsigned long nowMS = millis();

  if (nowMS - lastRPMms >= RPM_INTERVAL) {
    lastRPMms = nowMS;

    float pulsesPerSec = pulseCount * (1000.0 / RPM_INTERVAL);
    float rpm = pulsesPerSec * 60;

    pulseCount = 0;

    // Smoothing ringan (biar cepat & stabil)
    rpmFiltered = (alpha * rpm) + ((1 - alpha) * rpmFiltered);

    // OUTPUT RPM SAJA
    Serial.println(rpmFiltered, 2);
  }
}
