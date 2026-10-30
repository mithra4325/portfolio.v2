/*
 * ESP32 Voice-Controlled IoT Relay Actuation (IFTTT + Adafruit IO MQTT)
 * Assignment: IoT Task 3 — Cloud Automation Pipeline
 * Author: Mithra Ravi
 *
 * Description:
 * Connects the ESP32 to Adafruit IO via MQTT (port 1883) over local Wi-Fi.
 * Subscribes to the "bulb" feed which receives upstream automated commands 
 * from Google Assistant voice actions via IFTTT webhook triggers.
 *
 * Hardware Configuration:
 * - ESP32 NodeMCU / DevKit V4
 * - GPIO 23 -> Relay Module IN1 (Active-LOW: LOW = ON, HIGH = OFF)
 * - 5V Songle Relay Module switching a 230V AC incandescent lamp (COM/NO)
 */

#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// --- Wi-Fi Credentials (Placeholders: do not commit actual credentials) ---
#define WLAN_SSID      "YOUR_WIFI_NAME"
#define WLAN_PASS      "YOUR_WIFI_PASSWORD"

// --- Adafruit IO Cloud Broker Credentials (Placeholders) ---
#define AIO_SERVER     "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME   "YOUR_AIO_USERNAME"
#define AIO_KEY        "YOUR_AIO_KEY"

// --- Hardware Pin Definitions ---
const int RELAY_PIN = 23;   // GPIO connected to Relay Module IN1
const int RELAY_ON  = LOW;  // Active-LOW relay coil energized
const int RELAY_OFF = HIGH; // Active-LOW relay coil de-energized

// --- Networking & MQTT Client Setup ---
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup subscription feed for the bulb channel
Adafruit_MQTT_Subscribe bulbFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/bulb");

unsigned long lastPing = 0;

// Connects and reconnects to the Adafruit IO MQTT broker
void MQTT_connect() {
  if (mqtt.connected()) return;

  Serial.print("Connecting to Adafruit IO MQTT... ");
  int8_t ret;
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
    delay(5000); // Wait 5 seconds before retrying
    if (--retries == 0) {
      Serial.println("MQTT connection failed. Resetting ESP32...");
      ESP.restart();
    }
  }
  Serial.println("MQTT Connected!");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize relay pin in safe OFF state
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);

  Serial.println("\n--- Starting ESP32 Voice Automation Node ---");
  Serial.print("Connecting to Wi-Fi SSID: ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected successfully!");
  Serial.print("Local IP Address: ");
  Serial.println(WiFi.localIP());

  // Subscribe to Adafruit IO feed
  mqtt.subscribe(&bulbFeed);
  Serial.println("Subscribed to MQTT feed: " AIO_USERNAME "/feeds/bulb");
}

void loop() {
  // Ensure the MQTT connection is alive
  MQTT_connect();

  // Read incoming subscription packets with a 1000ms timeout
  Adafruit_MQTT_Subscribe *sub;
  while ((sub = mqtt.readSubscription(1000))) {
    if (sub == &bulbFeed) {
      String msg = (char *)bulbFeed.lastread;
      msg.trim();
      msg.toUpperCase();
      Serial.print("Voice automation command received: ");
      Serial.println(msg);

      if (msg == "ON") {
        digitalWrite(RELAY_PIN, RELAY_ON);
        Serial.println("Relay energized -> Lamp ON");
      } else if (msg == "OFF") {
        digitalWrite(RELAY_PIN, RELAY_OFF);
        Serial.println("Relay de-energized -> Lamp OFF");
      } else {
        Serial.print("Unrecognized payload received: ");
        Serial.println(msg);
      }
    }
  }

  // Send keepalive ping to broker every 30 seconds
  if (millis() - lastPing > 30000) {
    lastPing = millis();
    if (!mqtt.ping()) {
      Serial.println("MQTT ping failed; disconnecting...");
      mqtt.disconnect();
    }
  }
}
