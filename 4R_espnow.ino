#include <esp_now.h>
#include <WiFi.h>

// Define relay pins
#define RELAY_1_PIN 3
#define RELAY_2_PIN 10
#define RELAY_3_PIN 19
#define RELAY_4_PIN 5

// Define switch pins
#define SWITCH_1_PIN 2
#define SWITCH_2_PIN 6
#define SWITCH_3_PIN 4
#define SWITCH_4_PIN 18

// Relay states
bool relayStates[4] = {0, 0, 0, 0};

// Debounce variables
unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
const int debounceDelay = 50;  // 50ms debounce delay
bool lastSwitchStates[4] = {1, 1, 1, 1};
bool currentSwitchStates[4] = {1, 1, 1, 1};

// Structure to receive ESPNOW data
typedef struct {
  uint8_t relayNumber;
  uint8_t state;
} RelayCommand;

// ESPNOW receive callback (Updated for ESP32 Core 2.3.4)
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(RelayCommand)) {
    Serial.println("Invalid ESPNOW data received!");
    return;
  }

  RelayCommand command;
  memcpy(&command, incomingData, sizeof(command));

  // Validate relay number (1-4)
  if (command.relayNumber < 1 || command.relayNumber > 4) {
    Serial.println("Invalid relay number!");
    return;
  }

  int relayIndex = command.relayNumber - 1;  // Correct mapping
  int relayPins[4] = {RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN, RELAY_4_PIN};

  relayStates[relayIndex] = command.state;  // Update state
  digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);  // Control correct relay

  Serial.printf("ESPNOW: Relay %d -> %s\n", command.relayNumber, relayStates[relayIndex] ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);

  // Initialize relay pins
  int relayPins[4] = {RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN, RELAY_4_PIN};
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }

  // Initialize switch pins
  int switchPins[4] = {SWITCH_1_PIN, SWITCH_2_PIN, SWITCH_3_PIN, SWITCH_4_PIN};
  for (int i = 0; i < 4; i++) {
    pinMode(switchPins[i], INPUT_PULLUP);
  }

  // Initialize ESPNOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW & Manual Switches Ready!");
}

void loop() {
  int switchPins[4] = {SWITCH_1_PIN, SWITCH_2_PIN, SWITCH_3_PIN, SWITCH_4_PIN};
  int relayPins[4] = {RELAY_1_PIN, RELAY_2_PIN, RELAY_3_PIN, RELAY_4_PIN};

  for (int i = 0; i < 4; i++) {
    bool reading = digitalRead(switchPins[i]);  // Read current switch state

    // Debounce logic
    if (reading != currentSwitchStates[i]) {
      lastDebounceTime[i] = millis();
    }
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != lastSwitchStates[i]) {  // Act only on state change
        lastSwitchStates[i] = reading;
        if (reading == LOW) {  // Switch pressed (active-low)
          relayStates[i] = !relayStates[i];  // Toggle relay
          digitalWrite(relayPins[i], relayStates[i]);
          Serial.printf("Switch %d toggled: Relay %d is now %s\n", i + 1, i + 1, relayStates[i] ? "ON" : "OFF");
        }
      }
    }
    currentSwitchStates[i] = reading;
  }
}
