/*
 * ESP32 Smart Home IoT Node — Firebase Logging, Automation & Control
 * Assignment: IoT Task 5 — Full Edge-to-Cloud Integration
 * Author: Mithra Ravi
 *
 * Description:
 * Connects an ESP32 to Google Firebase Realtime Database over local Wi-Fi.
 * Gathers temperature and humidity from a DHT11 sensor, samples ambient light
 * from an LDR photoresistor divider, handles local debounced pushbutton overrides,
 * and controls an electrical relay module under both Manual and Automatic modes.
 *
 * Hardware Pinout:
 * - GPIO 4  -> DHT11 Single-Wire Data Pin
 * - GPIO 34 -> LDR Photoresistor Analog Divider (ADC1_CH6)
 * - GPIO 14 -> Tactile Pushbutton (Active-LOW, INPUT_PULLUP)
 * - GPIO 26 -> 5V Optocoupled Relay Module (Active-LOW: LOW=ON, HIGH=OFF)
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// --- Wi-Fi Credentials (Placeholders: do not commit actual credentials) ---
#define WIFI_SSID     "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// --- Firebase Project Credentials (Placeholders) ---
#define API_KEY       "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL  "https://YOUR_PROJECT-default-rtdb.firebaseio.com/"

// --- Hardware Pin Definitions ---
#define DHT_PIN    4
#define DHT_TYPE   DHT11
#define LDR_PIN    34
#define BUTTON_PIN 14
#define RELAY_PIN  26

// Active-LOW relay logic: LOW energizes coil, HIGH opens contact
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// Sensor & Firebase Instances
DHT dht(DHT_PIN, DHT_TYPE);
FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// State Variables
bool bulbState = false;
String currentMode = "manual"; // "manual" or "auto"
int lightThreshold = 1000;     // ADC raw threshold for automatic lighting

int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 250; // 250 ms time gate for debouncing

unsigned long lastSend = 0;
const unsigned long sendInterval = 5000; // Sample and transmit telemetry every 5 seconds

// Helper function to actuate the physical relay
void setBulb(bool on) {
  bulbState = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ON : RELAY_OFF);
}

// Stream callback: invoked when cloud values change
void streamCallback(FirebaseStream data) {
  String path = data.dataPath();
  
  if (path == "/bulb") {
    if (data.dataType() == "boolean") {
      setBulb(data.boolData());
      Serial.printf("Cloud update: bulb is now %s\n", bulbState ? "ON" : "OFF");
    } else if (data.dataType() == "int") {
      setBulb(data.intData() == 1);
      Serial.printf("Cloud update: bulb is now %s\n", bulbState ? "ON" : "OFF");
    }
  } 
  else if (path == "/mode" && data.dataType() == "string") {
    currentMode = data.stringData();
    currentMode.toLowerCase();
    Serial.printf("Cloud update: mode switched to %s\n", currentMode.c_str());
  } 
  else if (path == "/threshold" && data.dataType() == "int") {
    lightThreshold = data.intData();
    Serial.printf("Cloud update: light threshold set to %d\n", lightThreshold);
  }
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println("Firebase RTDB stream timed out, reconnecting...");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize sensors and actuators
  dht.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  setBulb(false); // Fail-safe: ensure lamp is OFF at boot

  Serial.println("\n--- Starting ESP32 Firebase Smart Home Node ---");
  Serial.print("Connecting to Wi-Fi SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected successfully!");
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());

  // Configure Firebase Client
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign in anonymously or with service account
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase client authentication successful.");
  } else {
    Serial.printf("Firebase sign-in failed: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Reconcile initial boot state from cloud database
  if (Firebase.RTDB.getBool(&fbdo, "/smartHome/bulb")) {
    setBulb(fbdo.boolData());
    Serial.printf("Reconciled initial bulb state: %s\n", bulbState ? "ON" : "OFF");
  }
  if (Firebase.RTDB.getString(&fbdo, "/smartHome/mode")) {
    currentMode = fbdo.stringData();
    currentMode.toLowerCase();
    Serial.printf("Reconciled initial mode: %s\n", currentMode.c_str());
  }
  if (Firebase.RTDB.getInt(&fbdo, "/smartHome/threshold")) {
    lightThreshold = fbdo.intData();
    Serial.printf("Reconciled initial light threshold: %d\n", lightThreshold);
  }

  // Begin continuous streaming on /smartHome path
  Firebase.RTDB.beginStream(&stream, "/smartHome");
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
  Serial.println("Subscribed to Firebase RTDB stream at /smartHome");
}

void loop() {
  unsigned long now = millis();

  // --- 1. LOCAL MANUAL OVERRIDE (Tactile Pushbutton with Debouncing) ---
  int reading = digitalRead(BUTTON_PIN);
  if (reading == LOW && lastButtonState == HIGH && (now - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = now;
    setBulb(!bulbState);
    Serial.printf("Manual button pressed -> Bulb switched %s\n", bulbState ? "ON" : "OFF");

    // Immediately synchronize local state change to cloud
    Firebase.RTDB.setBool(&fbdo, "/smartHome/bulb", bulbState);
  }
  lastButtonState = reading;

  // --- 2. PERIODIC SENSOR SAMPLING & AUTOMATION EVALUATION ---
  if (Firebase.ready() && (now - lastSend >= sendInterval)) {
    lastSend = now;

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();
    int ldrRaw = analogRead(LDR_PIN);
    int lightLux = map(ldrRaw, 0, 4095, 0, 1023); // Calibrated range

    if (!isnan(temp) && !isnan(hum)) {
      // Automatic Lighting Logic
      if (currentMode == "auto") {
        bool shouldTurnOn = (ldrRaw < lightThreshold);
        if (shouldTurnOn != bulbState) {
          setBulb(shouldTurnOn);
          Serial.printf("Auto mode: Light %d < Threshold %d -> Bulb %s\n", 
                        ldrRaw, lightThreshold, bulbState ? "ON" : "OFF");
          Firebase.RTDB.setBool(&fbdo, "/smartHome/bulb", bulbState);
        }
      }

      // Upload live sensor readings to Firebase Realtime Database
      Firebase.RTDB.setFloat(&fbdo, "/smartHome/temperature", temp);
      Firebase.RTDB.setFloat(&fbdo, "/smartHome/humidity", hum);
      Firebase.RTDB.setInt(&fbdo, "/smartHome/light", lightLux);

      // Also push to legacy /telemetry/live for multi-dashboard compatibility
      FirebaseJson json;
      json.set("temperature", temp);
      json.set("humidity", hum);
      json.set("light", lightLux);
      Firebase.RTDB.setJSON(&fbdo, "/telemetry/live", &json);

      Serial.printf("Telemetry Upload: %.1f C | %.1f %% | %d lux (ADC: %d) | Bulb: %s | Mode: %s\n", 
                    temp, hum, lightLux, ldrRaw, bulbState ? "ON" : "OFF", currentMode.c_str());
    } else {
      Serial.println("Warning: Failed to read from DHT11 sensor.");
    }
  }
}
