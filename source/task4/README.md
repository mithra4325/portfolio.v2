# Smart Home — Firebase IoT Monitoring Dashboard (Task 4)

A reactive cloud-connected IoT monitoring and control web application built with vanilla HTML5, CSS3, ES modules, and the official modular Firebase JavaScript SDK (v10).

## Features
- **Firebase Authentication**: User authentication via Email/Password and Google OAuth popup (`signInWithPopup`).
- **Real-Time Telemetry Streaming**: Subscribes to Firebase Realtime Database nodes (`smartHome/temperature`, `smartHome/humidity`, `smartHome/light`) with instantaneous DOM updates via `onValue()`.
- **Bi-Directional Actuator Control**: Cloud-synchronized ON/OFF control for an electrical bulb relay (`smartHome/bulb`), plus Manual/Automatic threshold automation.
- **ESP32 Heartbeat Watchdog**: Visual online/offline status badge with a 15-second heartbeat timer.
- **Historical Telemetry Visualization**: Responsive multi-axis line chart powered by Chart.js displaying temperature, humidity, and light levels over time.
- **CSV Data Export**: One-click download generating timestamped telemetry datasets (`smarthome_data.csv`).

## Project File Structure
```
source/task4/
├── index.html        # Main single-page application (Login + Dashboard screens)
├── style.css         # Clean, responsive UI styling and layout rules
├── script.js         # Firebase SDK v10 controller (Auth, RTDB, Charts, CSV)
└── README.md         # Setup and deployment documentation
```

## Setup & Deployment Instructions

### 1. Firebase Project Setup
1. Open the [Firebase Console](https://console.firebase.google.com/) and create a new project (e.g. `mithra-63101`).
2. Navigate to **Authentication** > **Sign-in method** and enable:
   - **Email/Password**
   - **Google**
3. Navigate to **Realtime Database** > **Create Database** (choose default location `us-central1`).
4. Set security rules for development or authenticated access:
```json
{
  "rules": {
    ".read": "auth != null",
    ".write": "auth != null"
  }
}
```

### 2. Configure Credentials
In `script.js`, replace the placeholder values in `firebaseConfig` with your Web App configuration credentials obtained from **Project Settings** > **General** > **Your apps**:
```javascript
const firebaseConfig = {
  apiKey: "YOUR_FIREBASE_API_KEY",
  authDomain: "YOUR_PROJECT_ID.firebaseapp.com",
  databaseURL: "https://YOUR_PROJECT_ID-default-rtdb.firebaseio.com",
  projectId: "YOUR_PROJECT_ID",
  storageBucket: "YOUR_PROJECT_ID.firebasestorage.app",
  messagingSenderId: "YOUR_SENDER_ID",
  appId: "YOUR_APP_ID"
};
```

### 3. Run Locally
Because the application uses ES modules (`type="module"`), serve the folder using a local web server:
```bash
# Using Python:
python -m http.server 8000

# Using Node.js npx:
npx serve .
```
Open `http://localhost:8000` in your web browser.

### 4. Deploy to Firebase Hosting
```bash
npm install -g firebase-tools
firebase login
firebase init hosting
firebase deploy --only hosting
```
Your live dashboard will be accessible globally via your Firebase web endpoint (e.g., `https://mithra-63101.web.app`).
