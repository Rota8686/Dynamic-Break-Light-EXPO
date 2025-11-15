#include <TFT_eSPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <SparkFun_TMAG5273_Arduino_Library.h>

//I2C configuration
#define SDA_PIN 23
#define SCL_PIN 22

//control inputs 
#define POWER_SWITCH_PIN 32
#define MODE_SWITCH_PIN 4
#define RESET_BUTTON_PIN 0
#define SIGNAL_A_PIN 27
#define SIGNAL_B_PIN 14

//Hall-Effect sensor calibration
float Z_baseline = -0.68;
float Z_high = 1.0;
float Z_low = 0.3;

//wheel configuration
#define WHEEL_DIAMETER_CM 66.0
#define WHEEL_CIRCUMFERENCE_CM (WHEEL_DIAMETER_CM * 3.14159)
#define WHEEL_CIRCUMFERENCE_M (WHEEL_CIRCUMFERENCE_CM / 100.0)
#define WHEEL_CIRCUMFERENCE_INCHES (WHEEL_CIRCUMFERENCE_CM / 2.54)

//constants for conversion
const float INCHES_PER_MILE = 63360.0;
const float METERS_PER_KM = 1000.0;
const int MAG_SAMPLES = 5;
const unsigned long VELOCITY_TIMEOUT_MS = 3000;

//data structure for receiving from sender
typedef struct struct_message_in {
  float accelValue;
  unsigned long timestamp;
} struct_message_in;

//data structure for sending to sender
typedef struct struct_message_out {
  bool buttonA;
  bool buttonB;
  unsigned long timestamp;
} struct_message_out;

struct_message_in incomingData;
struct_message_out outgoingData;

//sender MAC address
uint8_t senderAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//assigning variables
TFT_eSPI tft = TFT_eSPI();
TMAG5273 hallSensor;
esp_now_peer_info_t peerInfo;

float accelValue = 0.0;
float velocity_mps = 0.0; 
float velocity_kmh = 0.0;
float velocity_mph = 0.0;
float hallFieldStrength = 0.0;
float magZ_averaged = 0.0;
unsigned long lastPulseTime = 0;
unsigned long pulseInterval = 0;
unsigned long pulseCount = 0;
unsigned long lastDataReceived = 0;
unsigned long packetsReceived = 0;
float totalDistanceMiles = 0.0;
float totalDistanceKm = 0.0;
bool receivedData = false;
bool magnetDetected = false;
bool lastMagnetState = false;

//control variables
bool powerOn = false;
bool lastPowerState = false;
bool debugMode = false;
bool lastDebugMode = false;
bool signalA = false;
bool signalB = false;
bool lastSignalA = false;
bool lastSignalB = false;

//callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  accelValue = incomingData.accelValue;
  lastDataReceived = millis();
  receivedData = true;
  packetsReceived++;
}

//callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("button signal send failed");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nESP32 receiver - wireless display with TMAG5273");

  //initialize control inputs
  pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(SIGNAL_A_PIN, INPUT_PULLUP);
  pinMode(SIGNAL_B_PIN, INPUT_PULLUP);
  
  Serial.println("controls initialized");

  //start I2C 
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  //start Hall-Effect sensor
  if (hallSensor.begin(TMAG5273_I2C_ADDRESS_INITIAL, Wire) == 1) {
    Serial.println("TMAG5273 Hall-Effect sensor connected");
    Serial.print("baseline Z: ");
    Serial.print(Z_baseline, 3);
    Serial.print(" | high threshold: ");
    Serial.print(Z_high, 3);
    Serial.print(" | low threshold: ");
    Serial.println(Z_low, 3);
    Serial.print("wheel circumference: ");
    Serial.print(WHEEL_CIRCUMFERENCE_M, 3);
    Serial.print(" m (");
    Serial.print(WHEEL_CIRCUMFERENCE_INCHES, 2);
    Serial.println(" inches)");
  } else {
    Serial.println("error: TMAG5273 sensor not detected");
  }

  //set this controller as a WiFi station
  WiFi.mode(WIFI_STA);

  //print MAC address
  Serial.print("receiver MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("COPY THIS ADDRESS TO SENDER CODE!");

  //initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("error initializing ESP-NOW");
    return;
  }

  //register receive callback
  esp_now_register_recv_cb(OnDataRecv);
  
  //register send callback
  esp_now_register_send_cb(OnDataSent);
  
  //register peer
  memcpy(peerInfo.peer_addr, senderAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("failed to add peer");
  }

  //start TFT display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  //initial screen
  drawWelcomeScreen();

  Serial.println("system started");
  Serial.println("\n=== CONTROLS ===");
  Serial.println("power switch: toggle system on/off");
  Serial.println("mode switch: toggle debug/normal mode");
  Serial.println("reset button: hardware reset (connected to EN pin)");
  Serial.println("signal A button: send signal A via ESP-NOW");
  Serial.println("signal B button: send signal B via ESP-NOW");
}

void loop() {

  //read control inputs
  readControls();
  
  //check power state
  if (!powerOn) {
    if (lastPowerState) {
      showSleepScreen();
    }
    lastPowerState = powerOn;
    delay(100);
    return;
  }
  
  //system is on
  if (!lastPowerState) {
    tft.fillScreen(TFT_BLACK);
    if (debugMode) {
      drawDebugLabels();
    } else {
      drawNormalLabels();
    }
  }
  lastPowerState = powerOn;

  //check for data timeout
  if (millis() - lastDataReceived > 2000 && receivedData) {
    receivedData = false;
    Serial.println("data timeout - no signal from sender");
  }

  //read Hall-Effect sensor
  readHallSensor();

  //calculate velocity
  calculateVelocity();

  //update the display based on mode
  if (debugMode) {
    updateDebugDisplay();
  } else {
    updateNormalDisplay();
  }

  //debug output
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 2000 && debugMode) {
    Serial.print("Z: ");
    Serial.print(magZ_averaged, 3);
    Serial.print(" | Δ: ");
    Serial.print(magZ_averaged - Z_baseline, 3);
    Serial.print(" | passes: ");
    Serial.print(pulseCount);
    Serial.print(" | distance: ");
    Serial.print(totalDistanceMiles, 6);
    Serial.print(" mi (");
    Serial.print(totalDistanceKm, 3);
    Serial.print(" km) | speed: ");
    Serial.print(velocity_kmh, 1);
    Serial.println(" km/h");
    lastDebugPrint = millis();
  }

  delay(20);
}

//defining local functions

void readControls() {
  
  //read switches and buttons
  powerOn = !digitalRead(POWER_SWITCH_PIN);
  bool currentDebugMode = !digitalRead(MODE_SWITCH_PIN);
  signalA = !digitalRead(SIGNAL_A_PIN);
  signalB = !digitalRead(SIGNAL_B_PIN);
  
  //check for mode switch change
  if (currentDebugMode != lastDebugMode) {
    debugMode = currentDebugMode;
    Serial.print("mode switched to: ");
    Serial.println(debugMode ? "DEBUG" : "NORMAL");
    tft.fillScreen(TFT_BLACK);
    if (debugMode) {
      drawDebugLabels();
    } else {
      drawNormalLabels();
    }
  }
  lastDebugMode = currentDebugMode;
  
  //check for button presses and send via ESP-NOW
  if ((signalA && !lastSignalA) || (signalB && !lastSignalB)) {
    sendButtonSignals();
  }
  
  lastSignalA = signalA;
  lastSignalB = signalB;
}

void sendButtonSignals() {
  outgoingData.buttonA = signalA;
  outgoingData.buttonB = signalB;
  outgoingData.timestamp = millis();
  
  esp_err_t result = esp_now_send(senderAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
  
  if (result == ESP_OK) {
    Serial.print("button signal sent - A: ");
    Serial.print(signalA ? "ON" : "OFF");
    Serial.print(" B: ");
    Serial.println(signalB ? "ON" : "OFF");
  } else {
    Serial.println("error sending button signal");
  }
}

void readHallSensor() {
  
  //averaging for smoother readings
  float sum = 0;
  for (int i = 0; i < MAG_SAMPLES; i++) {
    sum += hallSensor.getZData();
    delayMicroseconds(100);
  }
  magZ_averaged = sum / MAG_SAMPLES;
  
  //calculate field strength
  hallFieldStrength = abs(magZ_averaged - Z_baseline);
  
  //magnet detection
  float delta = magZ_averaged - Z_baseline;
  
  if (delta > Z_high && !magnetDetected) {
    unsigned long currentTime = micros();
    
    if (currentTime - lastPulseTime > 100000) {
      pulseInterval = currentTime - lastPulseTime;
      lastPulseTime = currentTime;
      pulseCount++;
      magnetDetected = true;
      
      //calculate cumulative distance
      float distanceInches = pulseCount * WHEEL_CIRCUMFERENCE_INCHES;
      totalDistanceMiles = distanceInches / INCHES_PER_MILE;
      totalDistanceKm = (pulseCount * WHEEL_CIRCUMFERENCE_M) / METERS_PER_KM;
      
      Serial.print(">>> MAGNET DETECTED! Revolution ");
      Serial.print(pulseCount);
      Serial.print(" | delta: ");
      Serial.print(delta, 3);
      Serial.print(" | distance: ");
      Serial.print(totalDistanceKm, 3);
      Serial.println(" km");
    }
  } 
  else if (delta < Z_low && magnetDetected) {
    magnetDetected = false;
  }
  
  lastMagnetState = magnetDetected;
}

void calculateVelocity() {
  if (pulseCount == 0 || lastPulseTime == 0) {
    velocity_mps = 0.0;
    velocity_kmh = 0.0;
    velocity_mph = 0.0;
    return;
  }

  unsigned long timeSinceLastPulse = millis() - (lastPulseTime / 1000);
  if (timeSinceLastPulse > VELOCITY_TIMEOUT_MS) {
    velocity_mps = 0.0;
    velocity_kmh = 0.0;
    velocity_mph = 0.0;
    return;
  }

  if (pulseInterval > 0) {
    float timePerRev = pulseInterval / 1000000.0;

    velocity_mps = WHEEL_CIRCUMFERENCE_M / timePerRev;
    velocity_kmh = velocity_mps * 3.6;
    velocity_mph = velocity_kmh * 0.621371;

    if (velocity_kmh > 60.0) {
      Serial.println("WARNING: Unrealistic speed detected, zeroing velocity");
      velocity_kmh = 0.0;
      velocity_mps = 0.0;
      velocity_mph = 0.0;
    }
  }
}

void drawWelcomeScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(30, 100);
  tft.print("BIKE COMPUTER");
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 130);
  tft.print("Toggle power switch");
  tft.setCursor(40, 145);
  tft.print("to start system");
}

void showSleepScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(60, 150);
  tft.print("SLEEP");
}

void drawDebugLabels() {
  tft.setTextSize(2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  
  tft.setCursor(10, 10);
  tft.print("ACCEL:");
  
  tft.setCursor(10, 40);
  tft.print("HALL:");
  
  tft.setCursor(10, 70);
  tft.print("SPEED:");
  
  tft.setCursor(10, 110);
  tft.print("DISTANCE:");
  
  tft.setCursor(10, 160);
  tft.print("PULSES:");
  
  tft.setCursor(10, 190);
  tft.print("PACKETS:");
  
  tft.setCursor(10, 220);
  tft.print("WIFI:");
  
  tft.setCursor(10, 250);
  tft.print("SIGNALS:");
}

void drawNormalLabels() {
  tft.setTextSize(2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  
  tft.setCursor(10, 10);
  tft.print("SPEED:");
  
  tft.setCursor(10, 100);
  tft.print("DISTANCE:");
  
  tft.setCursor(10, 145);
  tft.print("ACCELERATION:");
}

void updateDebugDisplay() {
  
  //display accelerometer data
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(120, 10);
  if (receivedData) {
    tft.print(accelValue, 2);
    tft.print("    ");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("--  ");
  }

  //display Hall-Effect field strength
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.setCursor(120, 40);
  tft.print(hallFieldStrength, 2);
  if (magnetDetected) {
    tft.print("*");
  } else {
    tft.print(" ");
  }
  tft.print("   ");

  //display speed
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(120, 70);
  tft.print(velocity_kmh, 1);
  tft.print("    ");

  //display distance
  tft.setTextColor(TFT_RED, TFT_BLACK);  
  tft.setCursor(10, 130);
  tft.print(totalDistanceMiles, 3);
  tft.print(" mi   ");

  //display pulses
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(120, 160);
  tft.print(pulseCount);
  tft.print("     ");
  
  //display packets
  tft.setCursor(120, 190);
  tft.print(packetsReceived);
  tft.print("     ");

  //display WiFi status
  tft.setCursor(80, 220);
  if (receivedData) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("OK ");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("-- ");
  }
  
  //display button signals
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(130, 250);
  tft.print("A:");
  tft.print(signalA ? "1" : "0");
  tft.print(" B:");
  tft.print(signalB ? "1" : "0");
  tft.print("  ");
}

void updateNormalDisplay() {
  
  //display speed
  tft.setTextSize(4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(20, 40);
  if (velocity_mph >= 100.0) {
    tft.print((int)velocity_mph);
  } else if (velocity_mph >= 10.0) {
    tft.print(" ");
    tft.print((int)velocity_mph);
  } else {
    tft.print("  ");
    tft.print((int)velocity_mph);
  }
  tft.setTextSize(2);
  tft.print(" mph  ");

  //display distance
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  
  tft.setCursor(20, 120);
  tft.print(totalDistanceMiles, 2);
  tft.print(" mi  ");
  
  //draw acceleration circle
  drawAccelerationCircle();
  
  //show connection status
  tft.setTextSize(1);
  tft.setCursor(10, 290);
  if (receivedData) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("CONNECTED");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("NO SIGNAL");
  }
  tft.print("     ");
}

void drawAccelerationCircle() {
  float accel = abs(accelValue);
  int mappedValue = constrain(accel, 0, 3000);
  int radius = map(mappedValue, 0, 3000, 5, 65);
  
  int centerX = 120;
  int centerY = 225;
  
  static int lastRadius = 5;
  
  int radiusDiff = abs(radius - lastRadius);
  if (radiusDiff > 1) {
    
    if (lastRadius > 0) {
      tft.fillCircle(centerX, centerY, lastRadius + 2, TFT_BLACK);
    }
    
    tft.fillCircle(centerX, centerY, radius, TFT_RED);
    tft.drawCircle(centerX, centerY, radius + 1, TFT_WHITE);
    
    lastRadius = radius;
  }
  
  static float lastAccel = 0;
  if (abs(accel - lastAccel) > 50) {
    tft.fillRect(centerX - 20, centerY - 6, 40, 12, TFT_RED);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setCursor(centerX - 15, centerY - 4);
    tft.print(accel, 0);
    
    lastAccel = accel;
  }
}