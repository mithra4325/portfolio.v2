# ESP32 Firebase IoT System — Logging, Automation & Data Export (Task 5)

Complete edge-to-cloud IoT smart home solution featuring:
- **Transducer Telemetry Ingestion**: Continuous sampling of environmental temperature and humidity via DHT11, and ambient light intensity via an analog LDR voltage divider.
- **Bi-Directional Relay Actuation**: Cloud-synchronized switching of an electrical mains load under both Manual user control and Automatic threshold-driven lighting logic.
- **Physical Manual Override**: Local debounced tactile pushbutton on GPIO 14 that flips relay state and instantaneously synchronizes cloud database state.
- **Real-Time Web Dashboard**: Single-page web application featuring Firebase Authentication (Email/Password & Google OAuth), dynamic metric cards, and Chart.js historical analytics.
- **Automated Historical Logging**: Periodic 30-second automated snapshot archival to `smartHome/logs` with push IDs and server timestamps.
- **Telemetry CSV Export**: Client-side generation and download of structured `.csv` datasets (`smarthome_data.csv`).

---

## 1. Directory Structure

```
source/task5/
├── ESP32_Firebase_SmartHome.ino  # Production ESP32 firmware
├── index.html                    # Responsive web dashboard interface
├── script.js                     # Modular Firebase SDK v10 controller
├── style.css                     # Modern UI styling
├── SmartHome_Firebase_System.zip # Compressed full source package
└── README.md                     # Technical documentation & setup guide
```

---

## 2. Hardware Pinout & Wiring

| Component | Pin / Terminal | ESP32 Pin | Function / Notes |
|---|---|---|---|
| **DHT11 Sensor** | VCC | 3.3V / 5V | Sensor power rail |
| | GND | GND | Common ground |
| | DATA | GPIO 4 | Single-wire bidirectional digital data bus |
| **LDR Module / Divider** | VCC | 3.3V | Top of resistor divider (10kΩ pull-up) |
| | GND | GND | Bottom of photoresistor |
| | SIG / Mid-point | GPIO 34 | Analog-to-Digital Converter (ADC1_CH6) |
| **Tactile Pushbutton** | Leg 1 | GPIO 14 | Configured with internal `INPUT_PULLUP` |
| | Leg 2 | GND | Closes circuit to ground on press |
| **Relay Module** | VCC | 5V (VIN) | Coil supply voltage |
| | GND | GND | Common ground |
| | IN1 | GPIO 26 | Active-LOW control input (LOW = ON, HIGH = OFF) |
| **AC Load (Bulb)** | Mains Phase (L) | Relay COM | Line conductor from AC source |
| | Lamp Live Lead | Relay NO | Switched output to lamp fixture |
| | Mains Neutral (N) | Lamp Neutral Lead | Return conductor directly connected |

---

## 3. Firmware Setup & Compilation (Arduino IDE)

### Required Libraries
Install the following libraries via the Arduino IDE Library Manager:
1. **Firebase Arduino Client Library for ESP8266 and ESP32** (by Mobizt)
2. **DHT sensor library** (by Adafruit)
3. **Adafruit Unified Sensor** (by Adafruit)

### Credentials Configuration
In `ESP32_Firebase_SmartHome.ino`, replace placeholders with your network credentials and Firebase project details:
```cpp
#define WIFI_SSID     "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_KEY       "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL  "https://YOUR_PROJECT-default-rtdb.firebaseio.com/"
```

---

## 4. Firebase Cloud Setup

1. In the **Firebase Console**, create a project (e.g. `mithra-63101`).
2. Navigate to **Authentication** > **Sign-in method** and enable:
   - **Email/Password**
   - **Google**
3. Navigate to **Realtime Database** > **Create Database** (e.g. `us-central1`).
4. Set security rules allowing authenticated read/write access:
```json
{
  "rules": {
    ".read": "auth != null",
    ".write": "auth != null"
  }
}
```

---

## 5. Web Dashboard Execution

### Local Development
Because the web client uses native ES modules (`type="module"`), run with any static HTTP server:
```bash
# Using Python:
python -m http.server 8000

# Using Node.js npx:
npx serve .
```

### Production Deployment (Firebase Hosting)
```bash
npm install -g firebase-tools
firebase login
firebase init hosting
firebase deploy --only hosting
```
The application will be live globally at your Firebase Hosting URL (e.g. `https://mithra-63101.web.app`).
