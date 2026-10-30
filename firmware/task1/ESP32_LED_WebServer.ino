/*
 * ESP32 Local HTTP Web Server - LED Control
 * Assignment: IoT Task 1
 * Author: Mithra Ravi
 *
 * Description:
 * Configures the ESP32 as a Wi-Fi station (STA mode) hosting a lightweight HTTP
 * web server on port 80. Serves a responsive HTML control page allowing clients
 * on the local subnet to turn the onboard blue LED (GPIO 2) ON and OFF via
 * HTTP GET requests (/on and /off).
 */

#include <WiFi.h>
#include <WebServer.h>

// --- Wi-Fi Credentials (Replace with your local network details) ---
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// --- Hardware Pin Configuration ---
// ESP32 DevKit V4 routes the factory onboard blue LED to GPIO 2
const int ledPin = 2;

// Instantiate WebServer on standard HTTP port 80
WebServer server(80);

// Helper function to dynamically construct the HTML response page
String buildPage() {
  String state = digitalRead(ledPin) ? "ON" : "OFF";
  String page = "<!DOCTYPE html><html><head>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<title>ESP32 LED Control</title></head>";
  page += "<body style='text-align:center;font-family:sans-serif'>";
  page += "<h1>ESP32 LED Control</h1>";
  page += "<p>LED is " + state + "</p>";
  page += "<p><a href='/on'><button>ON</button></a> ";
  page += "<a href='/off'><button>OFF</button></a></p>";
  page += "</body></html>";
  return page;
}

// Route Handler: Root ("/") - Serves base control interface
void handleRoot() {
  server.send(200, "text/html", buildPage());
}

// Route Handler: Turn LED ON ("/on")
void handleOn() {
  digitalWrite(ledPin, HIGH); // Drive GPIO 2 to 3.3V (Turn LED ON)
  server.send(200, "text/html", buildPage());
}

// Route Handler: Turn LED OFF ("/off")
void handleOff() {
  digitalWrite(ledPin, LOW);  // Drive GPIO 2 to 0V (Turn LED OFF)
  server.send(200, "text/html", buildPage());
}

void setup() {
  Serial.begin(115200);

  // Configure GPIO 2 as digital output and set initial state to LOW (OFF)
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Connect to the local 2.4 GHz Wi-Fi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! Assigned IP address: ");
  Serial.println(WiFi.localIP());

  // Bind URL endpoints to their respective C++ callback functions
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);

  // Start the HTTP server listener socket
  server.begin();
  Serial.println("HTTP Web Server started on port 80.");
}

void loop() {
  // Listen for incoming client connections and handle requests
  server.handleClient();
}
