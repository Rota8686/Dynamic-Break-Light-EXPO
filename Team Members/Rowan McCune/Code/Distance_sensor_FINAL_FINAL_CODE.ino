#include <Wire.h>
#include "SparkFun_TMAG5273_Arduino_Library.h"

TMAG5273 sensor;

void setup() {
  Wire.begin();
  Serial.begin(500000); // Increased baud rate

  if (sensor.begin(TMAG5273_I2C_ADDRESS_INITIAL, Wire) != 1) {
    Serial.println("❌ Sensor failed to initialize");
    while (1);
  }

  Serial.println("✅ Hall Sensor Distance Tracker (Miles Version, 500000 baud)");
}

void loop() {
  static bool magnetDetected = false;
  static unsigned long passCount = 0;
  static unsigned long lastTime = 0;

  // ---- IMPERM VALS ----
  float Z_baseline = -0.68;   // Baseline reading without magnet
  float Z_high = 1.0;         // IMPERM VAL — Trigger threshold
  float Z_low  = 0.3;         // IMPERM VAL — Reset threshold
  float revDistance = 86.393; // Inches per revolution (IMPERM VAL)
  const float INCHES_PER_MILE = 63360.0;

  // ---- Averaging for smoother readings ----
  const int samples = 5;
  float sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += sensor.getZData();
    delay(1); // minimal delay for I2C stability
  }
  float magZ = sum / samples;

  // ---- Debug output ----
  Serial.print("Z = ");
  Serial.print(magZ, 3);
  Serial.print(" | Δ = ");
  Serial.print(magZ - Z_baseline, 3);

  // ---- Magnet detection ----
  if ((magZ - Z_baseline) > Z_high && !magnetDetected) {
    passCount++;
    magnetDetected = true;
    Serial.print("  >>> 🧲 MAGNET DETECTED! (Pass ");
    Serial.print(passCount);
    Serial.println(")");
  } 
  else if ((magZ - Z_baseline) < Z_low && magnetDetected) {
    magnetDetected = false;
    Serial.println();
  } 
  else {
    Serial.println();
  }

  // ---- Print distance every second ----
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {
    float distanceInches = passCount * revDistance;
    float distanceMiles = distanceInches / INCHES_PER_MILE;
    Serial.print("Total Passes: ");
    Serial.print(passCount);
    Serial.print(" | Distance: ");
    Serial.print(distanceMiles, 6);
    Serial.println(" miles");
    lastTime = currentTime;
  }

  delay(0); // run as fast as possible
}
