#include <esp_now.h>
#include <WiFi.h>

// RECEIVER MAC ADDRESS - Update with your receiver's actual MAC!
uint8_t receiverMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Accelerometer variables
int x, y, z;

// LED setup
const int LED_PIN = 12;
const int LED_CHANNEL = 0;

// Turn signal setup 
const int LEFT_SIGNAL_PIN = 14;
const int RIGHT_SIGNAL_PIN = 33;
const int LEFT_SIGNAL_CHANNEL = 1;   // Different PWM channel from brake light
const int RIGHT_SIGNAL_CHANNEL = 2;

// Data structure for SENDING to receiver (must match receiver's struct_message_in)
typedef struct struct_message_out {
  float accelValue;
  unsigned long timestamp;
} struct_message_out;

// Data structure for RECEIVING from receiver (must match receiver's struct_message_out)
typedef struct struct_message_in {
  bool buttonA;
  bool buttonB;
  unsigned long timestamp;
} struct_message_in;

struct_message_out outgoingData;
struct_message_in incomingData;

unsigned long lastTurnSignalBlink = 0;
bool turnSignalState = false;
const int TURN_SIGNAL_INTERVAL = 500;  // 500ms on, 500ms off 

// Timing variables
unsigned long lastSendTime = 0;
unsigned long packetsSent = 0;
const unsigned long sendInterval = 50;  // Send every 50ms (20Hz)

// Button state tracking
bool buttonAPressed = false;
bool buttonBPressed = false;

// Callback when data is received from receiver
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  
  buttonAPressed = incomingData.buttonA;
  buttonBPressed = incomingData.buttonB;
  
  Serial.print("Button signals received - A: ");
  Serial.print(buttonAPressed ? "PRESSED" : "released");
  Serial.print(" | B: ");
  Serial.println(buttonBPressed ? "PRESSED" : "released");
}

// Callback when data is sent to receiver
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("Send failed");
  }
}

void setup() {
  Serial.begin(460800);
  delay(1000);
  Serial.println("\n\nESP32 Accelerometer Transmitter + Brake Light");

  // Setup LED PWM
  ledcSetup(LED_CHANNEL, 5000, 8);
  ledcAttachPin(LED_PIN, LED_CHANNEL);

  // Print this ESP32's MAC address
  Serial.print("Sender MAC address: ");
  Serial.println(WiFi.macAddress());

  // Set device as WiFi station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  Serial.println("ESP-NOW initialized");

  // Register callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register receiver peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    Serial.println("UPDATE receiverMAC[] with correct address!");
    return;
  }

  Serial.println("Peer added successfully");
  Serial.println("\nReady to send data");
  Serial.println("================================\n");

  ledcSetup(LEFT_SIGNAL_CHANNEL, 5000, 8);
  ledcAttachPin(LEFT_SIGNAL_PIN, LEFT_SIGNAL_CHANNEL);
  
  ledcSetup(RIGHT_SIGNAL_CHANNEL, 5000, 8);
  ledcAttachPin(RIGHT_SIGNAL_PIN, RIGHT_SIGNAL_CHANNEL);
}

void loop() {
  yield();  // Feed watchdog

  // Read analog inputs from accelerometer
  x = analogRead(A1);
  y = analogRead(A0);
  z = analogRead(A2);

 
   updateTurnSignals();

  // Update brake light based on X acceleration
  updateBrakeLight(z);

  // Send data via ESP-NOW at specified interval (20Hz)
  if (millis() - lastSendTime >= sendInterval) {
    // Prepare data packet with acceleration in G-forces
    outgoingData.accelValue = z;
    outgoingData.timestamp = millis();

    // Send data via ESP-NOW
    esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));

    if (result == ESP_OK) {
      packetsSent++;
    } else {
      Serial.println("Error sending data");
    }

    lastSendTime = millis();
  }

  // Debug output every second
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    Serial.print("Z: "); Serial.print(z);
    Serial.print(" ("); Serial.print(z, 2); Serial.print("G)");
    Serial.print(" | Packets sent: "); Serial.print(packetsSent);
    Serial.print(" | Buttons A:"); Serial.print(buttonAPressed ? "1" : "0");
    Serial.print(" B:"); Serial.println(buttonBPressed ? "1" : "0");
    lastPrint = millis();
  }

  
}

void updateBrakeLight(int z) {
  static float smoothedBrightness = 0;  // Retains value between function calls
  static unsigned long lastFlashTime = 0;
  static bool flashState = false;
  int targetBrightness;
  
  // Emergency braking detection (below 1150)
  if (z < 1150) {
    // Flash pattern: 100ms on, 100ms off (non-blocking)
    if (millis() - lastFlashTime >= 100) {
      flashState = !flashState;
      lastFlashTime = millis();
    }
    
    if (flashState) {
      ledcWrite(LED_CHANNEL, 255);  // Flash ON
      delay (100);
      ledcWrite(LED_CHANNEL, 255);
      delay (100);
      ledcWrite(LED_CHANNEL, 255);
      delay (100);
      ledcWrite(LED_CHANNEL, 255);
      delay (100);
      ledcWrite(LED_CHANNEL, 255);
    } else {
      ledcWrite(LED_CHANNEL, 0);    // Flash OFF
    }
    
    // Reset smooth brightness for when we exit emergency mode
    smoothedBrightness = 255;
    return;
  }
  
  // Normal gradient braking (1150 to 2100)
  if (z >= 2100) {
    targetBrightness = 0;  // No braking
  }
  else if (z <= 1150) {
    targetBrightness = 255;  // Maximum braking (shouldn't reach here due to check above)
  }
  else {
    // Smooth gradient: map X value (2100 to 1150) to brightness (0 to 255)
    targetBrightness = map(z, 2100, 1150, 0, 255);
  }
  
  // Adaptive low-pass filter: faster response when braking starts
  float smoothingFactor;
  
  if (targetBrightness > smoothedBrightness) {
    // Braking harder (brightness increasing) - respond quickly!
    smoothingFactor = 0.5;  // 50% new value = fast response
  } else {
    // Releasing brake (brightness decreasing) - smooth fade
    smoothingFactor = 0.3;  // 30% new value = gradual fade
  }
  
  smoothedBrightness = smoothedBrightness * (1.0 - smoothingFactor) + targetBrightness * smoothingFactor;
  
  ledcWrite(LED_CHANNEL, (int)smoothedBrightness);
}

void updateTurnSignals() {
  // Non-blocking blink timer
  if (millis() - lastTurnSignalBlink >= TURN_SIGNAL_INTERVAL) {
    turnSignalState = !turnSignalState;
    lastTurnSignalBlink = millis();
  }
  
  // Control left turn signal with buttonA from receiver
  if (buttonAPressed) {
    if (turnSignalState) {
      ledcWrite(LEFT_SIGNAL_CHANNEL, 255);  // ON
    } else {
      ledcWrite(LEFT_SIGNAL_CHANNEL, 0);    // OFF
    }
  } else {
    ledcWrite(LEFT_SIGNAL_CHANNEL, 0);  // Turn off when button not pressed
  }
  
  // Control right turn signal with buttonB from receiver
  if (buttonBPressed) {
    if (turnSignalState) {
      ledcWrite(RIGHT_SIGNAL_CHANNEL, 255);  // ON
    } else {
      ledcWrite(RIGHT_SIGNAL_CHANNEL, 0);    // OFF
    }
  } else {
    ledcWrite(RIGHT_SIGNAL_CHANNEL, 0);  // Turn off when button not pressed
  }
}
