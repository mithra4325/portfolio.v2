/*
 * ESP32 Adafruit IO MQTT Control - Cloud Relay Actuation
 * Assignment: IoT Task 2
 * Author: Mithra Ravi
 *
 * Description:
 * Connects the ESP32 to Adafruit IO cloud via MQTT (port 1883).
 * Subscribes to the "bulb" feed to actuate an electrical relay and 230V AC bulb
 * from the cloud dashboard, with keepalive pings and automatic fault recovery.
 */

#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// --- Wi-Fi Credentials (Placeholders) ---
#define WLAN_SSID      "YOUR_WIFI_NAME"
#define WLAN_PASS      "YOUR_WIFI_PASSWORD"

// --- Adafruit IO Credentials (Placeholders) ---
#define AIO_SERVER     "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME   "YOUR_AIO_USERNAME"
#define AIO_KEY        "YOUR_AIO_KEY"

// --- Hardware Pin Configuration ---
const int OUTPUT_PIN = 23;   // Connected to Tongling Relay Module IN trigger pin
const int BUTTON_PIN = 4;    // Optional manual push button to GND

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup feeds: bulb subscription and bulb publication
Adafruit_MQTT_Subscribe bulbFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/bulb");
Adafruit_MQTT_Publish   bulbPub  = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bulb");

bool outputState = false;
int lastButton = HIGH;
unsigned long lastPress = 0;
unsigned long lastPing = 0;

void setOutput(bool on) {
  outputState = on;
  // Active-LOW relay logic: LOW energizes the coil, HIGH de-energizes
  digitalWrite(OUTPUT_PIN, on ? LOW : HIGH);
}

void MQTT_connect() {
  if (mqtt.connected()) return;
  Serial.print("Connecting to MQTT... ");
  int8_t ret;
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
    delay(5000);
    if (--retries == 0) ESP.restart();
  }
  Serial.println("connected");
}

void setup() {
  Serial.begin(115200);
  pinMode(OUTPUT_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  setOutput(false);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  mqtt.subscribe(&bulbFeed);
}

void loop() {
  MQTT_connect();

  // Message from the dashboard
  Adafruit_MQTT_Subscribe *sub;
  while ((sub = mqtt.readSubscription(100))) {
    if (sub == &bulbFeed) {
      String msg = (char *)bulbFeed.lastread;
      Serial.print("Received: ");
      Serial.println(msg);
      setOutput(msg == "ON");
    }
  }

  // Manual push button
  int reading = digitalRead(BUTTON_PIN);
  if (reading == LOW && lastButton == HIGH && millis() - lastPress > 250) {
    lastPress = millis();
    setOutput(!outputState);
    bulbPub.publish(outputState ? "ON" : "OFF");
  }
  lastButton = reading;

  // Keep the connection alive
  if (millis() - lastPing > 30000) {
    lastPing = millis();
    if (!mqtt.ping()) mqtt.disconnect();
  }
}
