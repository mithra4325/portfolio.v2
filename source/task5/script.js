// =========================================================
// SMART HOME - FIREBASE DASHBOARD CLIENT CONTROLLER
// Modular Firebase SDK v10 (Authentication & Realtime Database)
// =========================================================

import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-app.js";
import {
  getAuth,
  signInWithEmailAndPassword,
  signInWithPopup,
  GoogleAuthProvider,
  signOut,
  onAuthStateChanged
} from "https://www.gstatic.com/firebasejs/10.12.2/firebase-auth.js";
import {
  getDatabase,
  ref,
  onValue,
  set,
  push,
  query,
  limitToLast,
  serverTimestamp
} from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";

// =========================================================
// 1. FIREBASE CONFIGURATION (Placeholders for public repository)
// =========================================================
const firebaseConfig = {
  apiKey: "YOUR_FIREBASE_API_KEY",
  authDomain: "mithra-63101.firebaseapp.com",
  databaseURL: "https://mithra-63101-default-rtdb.firebaseio.com",
  projectId: "mithra-63101",
  storageBucket: "mithra-63101.firebasestorage.app",
  messagingSenderId: "649689915283",
  appId: "YOUR_FIREBASE_APP_ID",
  measurementId: "G-WQLV2VS42T"
};

// =========================================================
// 2. INITIALIZE FIREBASE
// =========================================================
let app, auth, db;

try {
  app = initializeApp(firebaseConfig);
  auth = getAuth(app);
  db = getDatabase(app);
  console.log("Firebase initialized successfully.");
} catch (error) {
  console.error("Firebase initialization failed:", error);
}

// =========================================================
// 3. DOM ELEMENTS
// =========================================================
const loginScreen = document.getElementById("login-screen");
const dashboardScreen = document.getElementById("dashboard-screen");

const loginForm = document.getElementById("login-form");
const emailInput = document.getElementById("email");
const passwordInput = document.getElementById("password");
const loginError = document.getElementById("login-error");
const logoutBtn = document.getElementById("logout-btn");

const tempValue = document.getElementById("temp-value");
const humidityValue = document.getElementById("humidity-value");
const lightValue = document.getElementById("light-value");
const lightBarFill = document.getElementById("light-bar-fill");
const lightLabel = document.getElementById("light-label");

const bulbStatus = document.getElementById("bulb-status");
const bulbIcon = document.getElementById("bulb-icon");
const bulbOnBtn = document.getElementById("bulb-on-btn");
const bulbOffBtn = document.getElementById("bulb-off-btn");

const espStatus = document.getElementById("esp-status");
const googleLoginBtn = document.getElementById("google-login-btn");

const manualModeBtn = document.getElementById("manual-mode-btn");
const autoModeBtn = document.getElementById("auto-mode-btn");
const thresholdSection = document.getElementById("threshold-section");
const thresholdInput = document.getElementById("threshold-input");
const saveThresholdBtn = document.getElementById("save-threshold-btn");

const exportCsvBtn = document.getElementById("export-csv-btn");
const historyTableBody = document.getElementById("history-table-body");
const historyChartCanvas = document.getElementById("history-chart");

// Helper safely setting text only if element exists
function setText(element, value) {
  if (element) {
    element.textContent = value;
  }
}

// =========================================================
// 4. AUTHENTICATION & LOGIN WORKFLOW
// =========================================================
if (loginForm) {
  loginForm.addEventListener("submit", (e) => {
    e.preventDefault();
    setText(loginError, "");

    const email = emailInput.value.trim();
    const password = passwordInput.value;

    signInWithEmailAndPassword(auth, email, password)
      .then(() => {
        loginForm.reset();
      })
      .catch((error) => {
        setText(loginError, "Incorrect email or password. Please try again.");
        console.error("Login error:", error.code, error.message);
      });
  });
}

if (googleLoginBtn) {
  const googleProvider = new GoogleAuthProvider();

  googleLoginBtn.addEventListener("click", () => {
    setText(loginError, "");

    signInWithPopup(auth, googleProvider)
      .then(() => {
        console.log("Google Sign-In successful");
      })
      .catch((error) => {
        setText(loginError, "Google Sign-In failed. Please try again.");
        console.error("Google login error:", error.code, error.message);
      });
  });
}

if (logoutBtn) {
  logoutBtn.addEventListener("click", () => {
    signOut(auth)
      .then(() => {
        console.log("User logged out");
      })
      .catch((error) => {
        console.error("Logout error:", error);
      });
  });
}

// Central state listener: switches screens based on auth status
onAuthStateChanged(auth, (user) => {
  if (user) {
    if (loginScreen) loginScreen.classList.add("hidden");
    if (dashboardScreen) dashboardScreen.classList.remove("hidden");
    console.log("Logged in as:", user.email);

    startListening();
    startLogging();
  } else {
    if (loginScreen) loginScreen.classList.remove("hidden");
    if (dashboardScreen) dashboardScreen.classList.add("hidden");
    console.log("No user signed in.");
  }
});

// =========================================================
// 5. CACHED LOCAL STATE
// =========================================================
const latestReading = {
  temperature: null,
  humidity: null,
  light: null,
  bulb: false,
  mode: "manual"
};

// =========================================================
// 6. REALTIME DATABASE LISTENERS
// =========================================================
let listenersAttached = false;

function startListening() {
  if (listenersAttached || !db) return;
  listenersAttached = true;

  // Temperature
  onValue(ref(db, "smartHome/temperature"), (snapshot) => {
    const val = snapshot.val();
    if (val !== null && val !== undefined) {
      latestReading.temperature = val;
      setText(tempValue, val);
      markEspOnline();
    }
  });

  // Humidity
  onValue(ref(db, "smartHome/humidity"), (snapshot) => {
    const val = snapshot.val();
    if (val !== null && val !== undefined) {
      latestReading.humidity = val;
      setText(humidityValue, val);
      markEspOnline();
    }
  });

  // Light
  onValue(ref(db, "smartHome/light"), (snapshot) => {
    const val = snapshot.val();
    if (val !== null && val !== undefined) {
      latestReading.light = val;
      setText(lightValue, val);
      updateLightBar(val);
      markEspOnline();
    }
  });

  // Bulb State
  onValue(ref(db, "smartHome/bulb"), (snapshot) => {
    const isOn = snapshot.val() === true;
    latestReading.bulb = isOn;
    updateBulbUI(isOn);
  });

  // Mode (manual / auto)
  onValue(ref(db, "smartHome/mode"), (snapshot) => {
    const mode = snapshot.val() || "manual";
    latestReading.mode = mode;
    updateModeUI(mode);
  });

  // Light Threshold
  onValue(ref(db, "smartHome/threshold"), (snapshot) => {
    const val = snapshot.val();
    if (val !== null && thresholdInput) {
      thresholdInput.value = val;
    }
  });

  // Historical Telemetry Logs (last 30 entries)
  const logsQuery = query(ref(db, "smartHome/logs"), limitToLast(30));
  onValue(logsQuery, (snapshot) => {
    const data = snapshot.val();
    if (!data) {
      renderHistoryTable([]);
      renderHistoryChart([]);
      return;
    }

    const logArray = Object.keys(data).map((key) => ({
      id: key,
      ...data[key]
    }));

    latestLogs = logArray;
    renderHistoryTable(logArray);
    renderHistoryChart(logArray);
  });
}

// =========================================================
// 7. UI UPDATE ROUTINES
// =========================================================
function updateBulbUI(isOn) {
  setText(bulbStatus, isOn ? "ON" : "OFF");
  if (bulbStatus) {
    bulbStatus.style.color = isOn ? "#16a34a" : "#dc2626";
  }
  if (bulbIcon) {
    bulbIcon.textContent = isOn ? "💡" : "⚪";
  }
}

function updateLightBar(lux) {
  if (!lightBarFill) return;
  const pct = Math.min(100, Math.round((lux / 1023) * 100));
  lightBarFill.style.width = pct + "%";

  if (lightLabel) {
    if (lux < 300) {
      lightLabel.textContent = "Dark";
    } else if (lux < 700) {
      lightLabel.textContent = "Moderate";
    } else {
      lightLabel.textContent = "Bright";
    }
  }
}

function updateModeUI(mode) {
  const isAuto = mode === "auto";

  if (manualModeBtn) manualModeBtn.classList.toggle("active", !isAuto);
  if (autoModeBtn) autoModeBtn.classList.toggle("active", isAuto);

  if (thresholdSection) {
    thresholdSection.classList.toggle("hidden", !isAuto);
  }

  // In auto mode, manual toggle buttons are disabled
  if (bulbOnBtn) bulbOnBtn.disabled = isAuto;
  if (bulbOffBtn) bulbOffBtn.disabled = isAuto;
}

// =========================================================
// 8. CONTROLS MUTATION HANDLERS
// =========================================================
if (bulbOnBtn) {
  bulbOnBtn.addEventListener("click", () => {
    set(ref(db, "smartHome/bulb"), true);
  });
}

if (bulbOffBtn) {
  bulbOffBtn.addEventListener("click", () => {
    set(ref(db, "smartHome/bulb"), false);
  });
}

if (manualModeBtn) {
  manualModeBtn.addEventListener("click", () => {
    set(ref(db, "smartHome/mode"), "manual");
  });
}

if (autoModeBtn) {
  autoModeBtn.addEventListener("click", () => {
    set(ref(db, "smartHome/mode"), "auto");
  });
}

if (saveThresholdBtn && thresholdInput) {
  saveThresholdBtn.addEventListener("click", () => {
    const val = parseInt(thresholdInput.value, 10);
    if (!isNaN(val) && val >= 0) {
      set(ref(db, "smartHome/threshold"), val)
        .then(() => {
          saveThresholdBtn.textContent = "Saved!";
          setTimeout(() => {
            saveThresholdBtn.textContent = "Save";
          }, 1500);
        })
        .catch((err) => {
          console.error("Error saving threshold:", err);
        });
    }
  });
}

// =========================================================
// 9. ESP32 HEARTBEAT WATCHDOG
// =========================================================
let espOfflineTimer = null;

function markEspOnline() {
  if (!espStatus) return;

  espStatus.classList.remove("offline");
  espStatus.classList.add("online");
  espStatus.innerHTML = '<span class="dot"></span> ESP32 Online';

  if (espOfflineTimer) clearTimeout(espOfflineTimer);
  espOfflineTimer = setTimeout(() => {
    espStatus.classList.remove("online");
    espStatus.classList.add("offline");
    espStatus.innerHTML = '<span class="dot"></span> ESP32 Offline';
  }, 15000);
}

// =========================================================
// 10. BACKGROUND DATA LOGGING
// =========================================================
let loggingInterval = null;

function startLogging() {
  if (loggingInterval) return;

  loggingInterval = setInterval(() => {
    if (latestReading.temperature === null) return;

    push(ref(db, "smartHome/logs"), {
      timestamp: serverTimestamp(),
      temperature: latestReading.temperature,
      humidity: latestReading.humidity,
      light: latestReading.light,
      bulb: latestReading.bulb,
      mode: latestReading.mode
    });
  }, 30000);
}

// =========================================================
// 11. HISTORY TABLE RENDERER
// =========================================================
let latestLogs = [];

function renderHistoryTable(logs) {
  if (!historyTableBody) return;

  if (!logs || logs.length === 0) {
    historyTableBody.innerHTML = '<tr><td colspan="6" class="empty-row">No data logged yet</td></tr>';
    return;
  }

  const sorted = [...logs].reverse();

  historyTableBody.innerHTML = sorted.map((log) => {
    const time = log.timestamp ? new Date(log.timestamp).toLocaleTimeString() : "--";
    return `<tr>
      <td>${time}</td>
      <td>${log.temperature ?? "--"}</td>
      <td>${log.humidity ?? "--"}</td>
      <td>${log.light ?? "--"}</td>
      <td>${log.bulb ? "ON" : "OFF"}</td>
      <td>${log.mode ?? "manual"}</td>
    </tr>`;
  }).join("");
}

// =========================================================
// 12. CHART.JS HISTORICAL LINE GRAPH
// =========================================================
let historyChart = null;

function renderHistoryChart(logs) {
  if (!historyChartCanvas || typeof Chart === "undefined") return;
  if (!logs || logs.length === 0) return;

  const labels = logs.map((log) =>
    log.timestamp ? new Date(log.timestamp).toLocaleTimeString() : ""
  );
  const temps = logs.map((log) => log.temperature ?? null);
  const humidities = logs.map((log) => log.humidity ?? null);
  const lights = logs.map((log) => log.light ?? null);

  if (historyChart) {
    historyChart.destroy();
  }

  historyChart = new Chart(historyChartCanvas, {
    type: "line",
    data: {
      labels: labels,
      datasets: [
        {
          label: "Temperature (°C)",
          data: temps,
          borderColor: "#ef4444",
          backgroundColor: "#ef4444",
          tension: 0.3
        },
        {
          label: "Humidity (%)",
          data: humidities,
          borderColor: "#3b82f6",
          backgroundColor: "#3b82f6",
          tension: 0.3
        },
        {
          label: "Light",
          data: lights,
          borderColor: "#f59e0b",
          backgroundColor: "#f59e0b",
          tension: 0.3,
          yAxisID: "y1"
        }
      ]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          type: "linear",
          position: "left",
          title: { display: true, text: "Temp / Humidity" }
        },
        y1: {
          type: "linear",
          position: "right",
          title: { display: true, text: "Light" },
          grid: { drawOnChartArea: false }
        }
      }
    }
  });
}

// =========================================================
// 13. CSV DATA EXPORT
// =========================================================
if (exportCsvBtn) {
  exportCsvBtn.addEventListener("click", () => {
    if (!latestLogs || latestLogs.length === 0) {
      alert("No historical data to export yet.");
      return;
    }

    const header = ["Timestamp", "Temperature", "Humidity", "Light", "Bulb", "Mode"];
    const rows = latestLogs.map((log) => [
      log.timestamp ? new Date(log.timestamp).toLocaleString() : "",
      log.temperature ?? "",
      log.humidity ?? "",
      log.light ?? "",
      log.bulb ? "ON" : "OFF",
      log.mode ?? "manual"
    ]);

    const csvContent = [header, ...rows].map((row) => row.join(",")).join("\n");
    const blob = new Blob([csvContent], { type: "text/csv" });
    const url = URL.createObjectURL(blob);

    const link = document.createElement("a");
    link.href = url;
    link.download = "smarthome_data.csv";
    link.click();

    URL.revokeObjectURL(url);
  });
}

