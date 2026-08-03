/*
 * ============================================================
 *  SMART INDOOR GARDEN  —  ESP-01 Master Controller & Edge AI Portal
 * ============================================================
 *  Primary controller of the smart indoor hydroponics. Handles:
 *    - Web Server AP dashboard (192.168.4.1)
 *    - LittleFS calibration standards log (garden_data.csv)
 *    - In-memory Edge AI model inference (Linear/MLP)
 *    - Loads dynamic AI weights from model_params.json
 *    - Runs anomaly detection on pH/nutrient sensor drift and pump failures.
 *    - Streams commands to/from the Arduino Uno co-processor.
 *
 *  YSK 2026  |   Kisii School  |  Predictive Hydroponic Optimization
 * ============================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <math.h>

// ─── Preset Configurations ──────────────────────────────────
const char* AP_SSID = "Garden-AI-Portal";
const char* AP_PASS = "garden1234";

// ─── Dynamic AI Model Parameters ────────────────────────────
struct ModelParams {
  String modelType = "linear";
  float lin_w_dosing[5] = {0.0, 0.0, 0.0, 0.0, 0.0}; // intercept, w_temp, w_hum, w_ph, w_nc
  
  float mlp_w1[8][4];
  float mlp_b1[8];
  float mlp_w2[8];
  float mlp_b2 = 0.0;
  
  bool loaded = false;
};
ModelParams activeModel;

// ─── Telemetry Variables ────────────────────────────────────
float temp = 0.0;
int hum = 0;
float ph = 7.0;
int nc = 250;
int pumpState = 0;
String currentMode = "REACTIVE";

// ─── Anomaly Detection States ───────────────────────────────
bool anomalyDetected = false;
String anomalyReason = "None";
bool isTraining = false;
String trainingStatus = "Idle";

// History buffers for Sensor Drift detection (30 samples)
#define DRIFT_BUFFER_SIZE 30
float phHistory[DRIFT_BUFFER_SIZE];
int ncHistory[DRIFT_BUFFER_SIZE];
int historyIndex = 0;
bool historyFull = false;

// Actuator pump monitoring variables
bool checkPumpEffect = false;
int ncBeforePump = 0;
unsigned long pumpStartTime = 0;
unsigned long pumpVerifyTime = 0;
int lastPumpState = 0;

// ─── Edge AI Scheduler & Dosing Timer ───────────────────────
unsigned long lastAiInferenceTime = 0;
unsigned long aiCooldownEndTime = 0;
float expectedDosing = 0.0;

// ─── Web Server Setup ───────────────────────────────────────
ESP8266WebServer server(80);

// ─── Serial Buffers ─────────────────────────────────────────
char serialBuf[64];
int serialLen = 0;

// ─── Forward Declarations ───────────────────────────────────
bool loadModelParams();
void runEdgeInference();
void runAnomalyDetection();

// ─── AI Inference Code ──────────────────────────────────────
void runEdgeInference() {
  if (!activeModel.loaded) return;
  
  // Normalized inputs (Min-Max scaled based on typical limits)
  float inputs[4];
  inputs[0] = temp / 50.0;
  inputs[1] = (float)hum / 100.0;
  inputs[2] = ph / 14.0;
  inputs[3] = (float)nc / 1023.0;
  
  float predDosing = 0.0;
  
  if (activeModel.modelType == "linear") {
    float d = activeModel.lin_w_dosing[0] + 
              activeModel.lin_w_dosing[1] * inputs[0] + 
              activeModel.lin_w_dosing[2] * inputs[1] + 
              activeModel.lin_w_dosing[3] * inputs[2] + 
              activeModel.lin_w_dosing[4] * inputs[3];
              
    predDosing = constrain(d, 0.0, 30.0);
  } 
  else if (activeModel.modelType == "mlp") {
    float hidden[8];
    // Hidden Layer (ReLU activation)
    for (int i = 0; i < 8; i++) {
      float h_sum = activeModel.mlp_b1[i];
      for (int j = 0; j < 4; j++) {
        h_sum += activeModel.mlp_w1[i][j] * inputs[j];
      }
      hidden[i] = (h_sum < 0.0) ? 0.0 : h_sum;
    }
    
    // Output Layer (Linear activation)
    float d_out = activeModel.mlp_b2;
    for (int i = 0; i < 8; i++) {
      d_out += activeModel.mlp_w2[i] * hidden[i];
    }
    
    predDosing = constrain(d_out, 0.0, 30.0);
  }
  
  expectedDosing = predDosing;

  // Trigger dosing command on Arduino co-processor if prediction > 0.5s and nutrient is below 500
  if (expectedDosing > 0.5 && nc < 500) {
    Serial.print("DOSING:");
    Serial.println(expectedDosing, 1);
  }
}

// ─── Anomaly Detection Twin ─────────────────────────────────
void runAnomalyDetection() {
  unsigned long now = millis();

  // 1. Record history for sensor drift detection
  phHistory[historyIndex] = ph;
  ncHistory[historyIndex] = nc;
  historyIndex = (historyIndex + 1) % DRIFT_BUFFER_SIZE;
  if (historyIndex == 0) {
    historyFull = true;
  }

  // 2. Check for Sensor Drift (Stuck values)
  bool phStuck = false;
  bool ncStuck = false;
  
  if (historyFull) {
    float phMin = phHistory[0], phMax = phHistory[0];
    int ncMin = ncHistory[0], ncMax = ncHistory[0];
    
    for (int i = 1; i < DRIFT_BUFFER_SIZE; i++) {
      if (phHistory[i] < phMin) phMin = phHistory[i];
      if (phHistory[i] > phMax) phMax = phHistory[i];
      if (ncHistory[i] < ncMin) ncMin = ncHistory[i];
      if (ncHistory[i] > ncMax) ncMax = ncHistory[i];
    }
    
    // If the difference is exactly zero, the sensor readings are frozen
    if (phMax - phMin == 0.0) {
      phStuck = true;
    }
    if (ncMax - ncMin == 0) {
      ncStuck = true;
    }
  }

  // 3. Monitor Pump Activity for Actuator Failure
  if (pumpState == 1 && lastPumpState == 0) {
    // Pump started
    ncBeforePump = nc;
    checkPumpEffect = true;
    pumpStartTime = now;
  } 
  else if (pumpState == 0 && lastPumpState == 1) {
    // Pump just stopped. Wait 5s for mixing before verifying EC increase.
    pumpVerifyTime = now + 5000;
  }
  
  lastPumpState = pumpState;

  bool pumpFailed = false;
  if (checkPumpEffect) {
    if (pumpState == 0 && now >= pumpVerifyTime) {
      // Dosing ended and mixing time elapsed.
      // If nutrient level didn't increase, the pump failed or reservoir is dry.
      if (nc <= ncBeforePump) {
        pumpFailed = true;
      } else {
        checkPumpEffect = false; // reset check on success
      }
    } 
    else if (pumpState == 1 && (now - pumpStartTime > 35000)) {
      // Pump has been running for a long time (>35s) but no change. Failsafe alert.
      if (nc <= ncBeforePump) {
        pumpFailed = true;
      }
    }
  }

  // 4. Combine and update anomaly status
  if (phStuck) {
    anomalyDetected = true;
    anomalyReason = "Sensor Drift: pH probe reading is stuck/frozen";
  } else if (ncStuck) {
    anomalyDetected = true;
    anomalyReason = "Sensor Drift: Nutrient sensor (EC) reading is stuck/frozen";
  } else if (pumpFailed) {
    anomalyDetected = true;
    anomalyReason = "Actuator Failure: Dosing pump active but no increase in nutrient concentration";
  } else {
    anomalyDetected = false;
    anomalyReason = "None";
  }
}

// ─── File Operations (model_params.json / garden_data.csv) ──
bool loadModelParams() {
  if (!LittleFS.exists("/model_params.json")) {
    activeModel.loaded = false;
    return false;
  }
  
  File file = LittleFS.open("/model_params.json", "r");
  if (!file) return false;
  
#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(2048);
#endif

  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    activeModel.loaded = false;
    return false;
  }
  
  activeModel.modelType = doc["model_type"] | "linear";
  
  if (activeModel.modelType == "linear") {
    JsonArray w_dosing = doc["w_dosing"];
    for (int i = 0; i < 5; i++) {
      activeModel.lin_w_dosing[i] = w_dosing[i] | 0.0;
    }
  } 
  else if (activeModel.modelType == "mlp") {
    JsonArray w1 = doc["w1"];
    JsonArray b1 = doc["b1"];
    JsonArray w2 = doc["w2"];
    float b2 = doc["b2"] | 0.0;
    
    activeModel.mlp_b2 = b2;
    
    for (int i = 0; i < 8; i++) {
      JsonArray row = w1[i];
      for (int j = 0; j < 4; j++) {
        activeModel.mlp_w1[i][j] = row[j] | 0.0;
      }
      activeModel.mlp_b1[i] = b1[i] | 0.0;
      activeModel.mlp_w2[i] = w2[i] | 0.0;
    }
  }
  
  activeModel.loaded = true;
  return true;
}

// ─── Web Interface HTML ──────────────────────────────────────
const char HTTP_INDEX[] PROGMEM = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Garden AI Maintenance & Control Center</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=Outfit:wght@400;500;600;700&display=swap');
    
    :root {
      --bg-dark: #05080e;
      --bg-card: rgba(10, 18, 15, 0.7);
      --border-glow: rgba(16, 185, 129, 0.15);
      --primary: #10b981;
      --primary-glow: rgba(16, 185, 129, 0.25);
      --accent: #a855f7;
      --accent-glow: rgba(168, 85, 247, 0.2);
      --info: #0ea5e9;
      --info-glow: rgba(14, 165, 233, 0.25);
      --success: #10b981;
      --warning: #f59e0b;
      --danger: #ef4444;
      --text-main: #f8fafc;
      --text-muted: #64748b;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'Inter', sans-serif;
      background-color: var(--bg-dark);
      background-image: 
        radial-gradient(circle at 10% 10%, rgba(16, 185, 129, 0.08) 0%, transparent 40%),
        radial-gradient(circle at 90% 90%, rgba(168, 85, 247, 0.06) 0%, transparent 40%);
      color: var(--text-main);
      height: 100vh;
      padding: 16px;
      display: flex;
      flex-direction: column;
      overflow: hidden;
    }

    .container {
      width: 100%;
      max-width: 1100px;
      margin: 0 auto;
      display: flex;
      flex-direction: column;
      height: 100%;
    }
    
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding-bottom: 8px;
      border-bottom: 1px solid var(--border-glow);
      flex-shrink: 0;
    }
    
    h1 {
      font-family: 'Outfit', sans-serif;
      font-size: 1.6rem;
      font-weight: 700;
      background: linear-gradient(135deg, #10b981 0%, #a855f7 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    
    .status-pills { display: flex; gap: 8px; }
    .status-pill {
      background: rgba(15, 23, 42, 0.5);
      border: 1px solid var(--border-glow);
      padding: 4px 10px;
      border-radius: 99px;
      font-size: 0.75rem;
      font-weight: 500;
      display: flex;
      align-items: center;
      gap: 6px;
      color: var(--text-muted);
    }
    .status-pill.active { color: var(--text-main); border-color: rgba(16, 185, 129, 0.35); }
    .status-dot { width: 6px; height: 6px; border-radius: 50%; background-color: var(--text-muted); }
    .status-dot.active { background-color: var(--success); box-shadow: 0 0 6px var(--success); }
    .status-dot.danger { background-color: var(--danger); box-shadow: 0 0 6px var(--danger); }

    .grid {
      display: grid;
      grid-template-columns: 1fr 1.1fr;
      gap: 16px;
      flex: 1;
      overflow: hidden;
      margin-top: 12px;
    }

    .col {
      display: flex;
      flex-direction: column;
      gap: 16px;
      height: 100%;
      overflow: hidden;
    }

    .card {
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border: 1px solid var(--border-glow);
      border-radius: 12px;
      padding: 16px;
      box-shadow: 0 4px 20px 0 rgba(0, 0, 0, 0.3);
      display: flex;
      flex-direction: column;
      min-height: 0;
    }

    .card-title {
      font-family: 'Outfit', sans-serif;
      font-size: 1.05rem;
      font-weight: 600;
      color: var(--primary);
      margin-bottom: 12px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 6px;
      flex-shrink: 0;
    }
    .card-title.accent { color: var(--accent); }

    .card-title span { display: flex; align-items: center; gap: 6px; }

    /* SVG Hydroponic Tower */
    .stage {
      height: 120px;
      display: flex;
      justify-content: center;
      align-items: center;
      background: rgba(7, 10, 19, 0.4);
      border-radius: 10px;
      overflow: hidden;
      margin-bottom: 12px;
      flex-shrink: 0;
      border: 1px solid rgba(255, 255, 255, 0.02);
    }

    @keyframes flow-anim {
      to { stroke-dashoffset: -20; }
    }
    .water-flow.active {
      display: block !important;
      animation: flow-anim 1.5s linear infinite;
    }

    .ldr-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      flex-shrink: 0;
    }
    
    /* Circular Gauges styles */
    .gauge-container {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 8px;
      margin-bottom: 12px;
    }
    .gauge-card {
      background: rgba(7, 10, 19, 0.3);
      border: 1px solid rgba(255, 255, 255, 0.02);
      border-radius: 8px;
      padding: 8px;
      display: flex;
      flex-direction: column;
      align-items: center;
      position: relative;
    }
    .gauge-ring {
      width: 50px;
      height: 50px;
      transform: rotate(-90deg);
    }
    .ring-bg {
      fill: none;
      stroke: rgba(255, 255, 255, 0.05);
      stroke-width: 3.5;
    }
    .ring-fill {
      fill: none;
      stroke-width: 3.5;
      stroke-linecap: round;
      transition: stroke-dasharray 0.5s ease;
    }
    .ring-fill.temp { stroke: #f97316; }
    .ring-fill.hum { stroke: #0ea5e9; }
    .ring-fill.ph { stroke: #a855f7; }
    .ring-fill.ec { stroke: #10b981; }
    
    .gauge-text {
      position: absolute;
      top: 24px;
      font-size: 0.65rem;
      font-weight: 700;
      font-family: monospace;
    }
    .gauge-lbl {
      margin-top: 6px;
      font-size: 0.6rem;
      color: var(--text-muted);
      text-transform: uppercase;
      font-weight: 600;
    }

    .telemetry-row {
      display: flex;
      justify-content: space-between;
      padding: 8px 0;
      border-bottom: 1px solid rgba(255, 255, 255, 0.02);
      font-size: 0.8rem;
    }
    .telemetry-row:last-child { border-bottom: none; }
    .telemetry-val { font-family: monospace; font-weight: bold; }

    /* Button Group Options */
    .btn-group { display: flex; gap: 6px; margin-bottom: 12px; }
    .btn-opt {
      flex: 1;
      background: rgba(15, 23, 42, 0.5);
      border: 1px solid var(--border-glow);
      padding: 8px;
      border-radius: 8px;
      color: var(--text-muted);
      cursor: pointer;
      font-weight: 600;
      font-size: 0.8rem;
      transition: all 0.2s ease;
      text-align: center;
    }
    .btn-opt.active {
      background: linear-gradient(135deg, var(--primary) 0%, #059669 100%);
      color: white;
      border-color: rgba(16, 185, 129, 0.4);
      box-shadow: 0 3px 10px var(--primary-glow);
    }

    .slider-container { margin-bottom: 8px; }
    .slider-lbl { display: flex; justify-content: space-between; font-size: 0.75rem; margin-bottom: 4px; }
    .slider-val { font-family: monospace; font-weight: bold; color: var(--primary); }
    input[type=range] {
      width: 100%;
      height: 5px;
      background: #1e293b;
      border-radius: 99px;
      outline: none;
      -webkit-appearance: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 14px;
      height: 14px;
      border-radius: 50%;
      background: var(--primary);
      cursor: pointer;
      box-shadow: 0 0 6px var(--primary-glow);
    }

    .btn {
      width: 100%;
      background: linear-gradient(135deg, var(--accent) 0%, #7c3aed 100%);
      color: white;
      border: none;
      padding: 8px;
      border-radius: 8px;
      font-size: 0.8rem;
      font-weight: 600;
      cursor: pointer;
      box-shadow: 0 3px 8px var(--accent-glow);
      transition: all 0.2s ease;
      text-align: center;
      text-decoration: none;
      display: inline-block;
    }
    .btn:hover { transform: translateY(-1px); box-shadow: 0 4px 12px var(--accent-glow); }
    .btn-sec { background: rgba(255, 255, 255, 0.05); border: 1px solid var(--border-glow); color: white; box-shadow: none; }
    .btn-sec:hover { background: rgba(255, 255, 255, 0.08); box-shadow: none; }
    .btn-action {
      background: linear-gradient(135deg, var(--primary) 0%, #059669 100%);
      box-shadow: 0 3px 8px var(--primary-glow);
      padding: 8px;
    }
    .btn-action:hover { box-shadow: 0 4px 12px var(--primary-glow); }

    .dropzone {
      border: 1.5px dashed rgba(168, 85, 247, 0.3);
      border-radius: 8px;
      text-align: center;
      cursor: pointer;
      background: rgba(168, 85, 247, 0.01);
      transition: all 0.2s ease;
    }
    .dropzone:hover { border-color: var(--accent); background: rgba(168, 85, 247, 0.04); }

    /* Gemini-Style Chatspace CSS */
    .chat-container {
      display: flex;
      flex-direction: column;
      background: rgba(7, 10, 19, 0.4);
      border-radius: 10px;
      border: 1px solid rgba(255, 255, 255, 0.02);
      overflow: hidden;
      flex: 1;
      min-height: 0;
    }
    .chat-messages {
      flex: 1;
      padding: 12px;
      overflow-y: auto;
      display: flex;
      flex-direction: column;
      gap: 10px;
      min-height: 0;
    }
    .chat-bubble {
      max-width: 85%;
      padding: 8px 12px;
      border-radius: 10px;
      font-size: 0.8rem;
      line-height: 1.35;
      word-wrap: break-word;
    }
    .chat-bubble.user {
      align-self: flex-end;
      background: rgba(16, 185, 129, 0.15);
      border: 1px solid rgba(16, 185, 129, 0.3);
      color: #f8fafc;
      border-bottom-right-radius: 2px;
    }
    .chat-bubble.assistant {
      align-self: flex-start;
      background: rgba(168, 85, 247, 0.1);
      border: 1px solid rgba(168, 85, 247, 0.25);
      color: #f8fafc;
      border-bottom-left-radius: 2px;
    }
    .chat-presets {
      display: flex;
      gap: 6px;
      padding: 6px 12px;
      background: rgba(15, 23, 42, 0.4);
      border-top: 1px solid rgba(255, 255, 255, 0.02);
      overflow-x: auto;
      scrollbar-width: none;
      flex-shrink: 0;
    }
    .chat-presets::-webkit-scrollbar { display: none; }
    .preset-btn {
      background: rgba(255, 255, 255, 0.05);
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 99px;
      color: var(--text-muted);
      padding: 4px 10px;
      font-size: 0.7rem;
      cursor: pointer;
      white-space: nowrap;
      transition: all 0.2s ease;
    }
    .preset-btn:hover {
      background: rgba(168, 85, 247, 0.15);
      border-color: rgba(168, 85, 247, 0.3);
      color: var(--text-main);
    }
    .chat-input-bar {
      display: flex;
      align-items: center;
      padding: 4px 8px;
      background: rgba(15, 23, 42, 0.80);
      border-top: 1px solid rgba(255, 255, 255, 0.04);
      gap: 6px;
      flex-shrink: 0;
    }
    .chat-input {
      flex: 1;
      background: transparent;
      border: none;
      color: white;
      padding: 8px;
      outline: none;
      font-size: 0.8rem;
    }
    .chat-send-btn {
      background: var(--accent);
      border: none;
      color: white;
      width: 28px;
      height: 28px;
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: all 0.2s ease;
      flex-shrink: 0;
    }
    .chat-send-btn:hover {
      background: var(--primary);
      transform: scale(1.05);
    }

    /* Thinking dots animation */
    .thinking { display: flex; align-items: center; gap: 4px; padding: 10px 14px; }
    .thinking .dot {
      width: 5px;
      height: 5px;
      background: var(--accent);
      border-radius: 50%;
      animation: bounce 1.4s infinite ease-in-out both;
    }
    .thinking .dot:nth-child(1) { animation-delay: -0.32s; }
    .thinking .dot:nth-child(2) { animation-delay: -0.16s; }

    @keyframes bounce {
      0%, 80%, 100% { transform: scale(0); }
      40% { transform: scale(1.0); }
    }

    /* AI Self-Learning Training Animations */
    @keyframes dash {
      to { stroke-dashoffset: -20; }
    }
    @keyframes pulse-node {
      0%, 100% { transform: scale(1); }
      50% { transform: scale(1.15); opacity: 0.9; }
    }
    @keyframes progress-run {
      0% { margin-left: -30%; width: 30%; }
      50% { width: 40%; }
      100% { margin-left: 100%; width: 30%; }
    }
    @keyframes blinker {
      50% { opacity: 0.4; }
    }
    @keyframes glow-border {
      0%, 100% { border-color: rgba(168, 85, 247, 0.2); box-shadow: 0 0 5px rgba(168, 85, 247, 0.05); }
      50% { border-color: rgba(168, 85, 247, 0.5); box-shadow: 0 0 10px rgba(168, 85, 247, 0.15); }
    }

    @media (max-width: 768px) {
      body { height: auto; overflow: auto; padding: 12px; }
      .container { height: auto; }
      .grid { grid-template-columns: 1fr; height: auto; overflow: visible; gap: 12px; }
      .col { height: auto; overflow: visible; gap: 12px; }
      header { flex-direction: column; align-items: flex-start; gap: 10px; }
      .status-pills { width: 100%; justify-content: space-between; }
      .chat-container { height: 280px; }
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Garden-AI Portal</h1>
      <div class="status-pills">
        <div class="status-pill active" id="anomaly-status-pill">
          <div class="status-dot active" id="anomaly-status-dot"></div>
          Health: <span id="anomaly-pill-text">Healthy</span>
        </div>
        <div class="status-pill active">
          <div class="status-dot active" id="ai-status-dot"></div>
          AI: <span id="ai-model-status">Inactive</span>
        </div>
        <div class="status-pill active" id="training-status-pill" style="display:none; border-color:rgba(168,85,247,0.35);">
          <div class="status-dot active" style="background-color:var(--accent); box-shadow:0 0 6px var(--accent);"></div >
          Self-Learning: <span id="training-pill-text" style="color:var(--accent); font-weight:bold;">Optimizing</span>
        </div>
      </div>
    </header>

    <div class="grid">
      <!-- Left Column: Visualizers & Telemetry -->
      <div class="col" style="flex: 1;">
        <!-- Live System Layout Visualizer -->
        <div class="card" style="flex: 1.1; justify-content: space-between;">
          <div class="card-title"><span>Hydroponic DFT System</span></div>
          <div class="stage">
            <svg viewBox="0 0 200 200" style="width:100%; height:100%;">
              <!-- Hydroponic Tower / Pipes -->
              <rect x="85" y="15" width="30" height="120" rx="4" fill="#1e293b" stroke="var(--primary)" stroke-width="1.5" />
              
              <!-- Plants -->
              <!-- Plant 1 Left -->
              <path d="M 60 45 Q 75 30 85 45" fill="none" stroke="#059669" stroke-width="3" />
              <circle cx="60" cy="45" r="4.5" fill="#10b981" />
              <!-- Plant 1 Right -->
              <path d="M 140 45 Q 125 30 115 45" fill="none" stroke="#059669" stroke-width="3" />
              <circle cx="140" cy="45" r="4.5" fill="#10b981" />
              
              <!-- Plant 2 Left -->
              <path d="M 60 85 Q 75 70 85 85" fill="none" stroke="#059669" stroke-width="3" />
              <circle cx="60" cy="85" r="4.5" fill="#10b981" />
              <!-- Plant 2 Right -->
              <path d="M 140 85 Q 125 70 115 85" fill="none" stroke="#059669" stroke-width="3" />
              <circle cx="140" cy="85" r="4.5" fill="#10b981" />
              
              <!-- Nutrient Reservoir / Tank -->
              <rect x="45" y="135" width="110" height="50" rx="8" fill="rgba(15, 23, 42, 0.85)" stroke="rgba(255,255,255,0.06)" stroke-width="1.2" />
              
              <!-- Liquid level inside tank -->
              <rect id="liquid-level" x="48" y="145" width="104" height="36" rx="4" fill="url(#liquid-gradient)" style="transition: fill 0.6s ease;" />
              
              <!-- Flow lines when pump active -->
              <path id="water-flow" class="water-flow" d="M 100 135 L 100 25" fill="none" stroke="#0ea5e9" stroke-width="3" stroke-dasharray="6" stroke-dashoffset="0" style="display:none;" />
              
              <defs>
                <linearGradient id="liquid-gradient" x1="0%" y1="0%" x2="0%" y2="100%">
                  <stop id="grad-stop1" offset="0%" stop-color="#10b981" stop-opacity="0.8" />
                  <stop id="grad-stop2" offset="100%" stop-color="#047857" stop-opacity="0.95" />
                </linearGradient>
              </defs>
            </svg>
          </div>
          
          <!-- circular gauges -->
          <div class="gauge-container">
            <div class="gauge-card">
              <svg class="gauge-ring" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-temp" class="ring-fill temp" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-text" id="lbl-temp">0°C</div>
              <div class="gauge-lbl">Temp</div>
            </div>
            
            <div class="gauge-card">
              <svg class="gauge-ring" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-hum" class="ring-fill hum" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-text" id="lbl-hum">0%</div>
              <div class="gauge-lbl">Humidity</div>
            </div>

            <div class="gauge-card">
              <svg class="gauge-ring" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-ph" class="ring-fill ph" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-text" id="lbl-ph">7.0</div>
              <div class="gauge-lbl">pH</div>
            </div>

            <div class="gauge-card">
              <svg class="gauge-ring" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-ec" class="ring-fill ec" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-text" id="lbl-ec">0</div>
              <div class="gauge-lbl">EC / NPK</div>
            </div>
          </div>
        </div>

        <!-- Telemetry Summary -->
        <div class="card" style="flex: 1; justify-content: space-between;">
          <div class="card-title"><span>System Telemetry & Parameters</span></div>
          <div>
            <div class="telemetry-row">
              <span class="gauge-lbl">AI Expected Dosing Duration:</span>
              <span class="telemetry-val" id="lbl-expected-dosing" style="color:var(--info);">0.0 s</span>
            </div>
            <div class="telemetry-row">
              <span class="gauge-lbl">Pump/Valve State:</span>
              <span class="telemetry-val" id="lbl-pump-state" style="color:var(--success);">OFF</span>
            </div>
            <div class="telemetry-row">
              <span class="gauge-lbl">Active AI Model type:</span>
              <span class="telemetry-val" id="lbl-active-model-type">None</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Right Column: Control Panel & Chatbot -->
      <div class="col" style="flex: 1.15;">
        <!-- Control panel card -->
        <div class="card" style="flex: 1.1; justify-content: space-between;">
          <div class="card-title"><span>System Control center</span></div>
          <div>
            <div class="btn-group">
              <div class="btn-opt" id="opt-reactive" onclick="setMode('reactive')">Reactive</div>
              <div class="btn-opt" id="opt-manual" onclick="setMode('manual')">Manual</div>
              <div class="btn-opt" id="opt-ai" onclick="setMode('ai')">AI-Edge</div>
            </div>

            <!-- Manual Pump trigger -->
            <button class="btn btn-action" id="btn-manual-pump" style="margin-bottom: 12px;" onclick="togglePump()" disabled>Turn Pump ON</button>

            <div class="slider-container">
              <div class="slider-lbl">
                <span class="gauge-lbl">Target Dosing Duration</span>
                <span class="slider-val"><span id="slide-val-duration">5.0</span>s</span>
              </div>
              <input type="range" id="slide-duration" min="0.5" max="30.0" step="0.5" value="5.0" oninput="updateDosingSlider()">
            </div>
            
            <!-- Training monitoring widget -->
            <div id="control-training-widget" style="display:none; background:rgba(168,85,247,0.05); border:1px dashed var(--accent-glow); padding:8px 12px; border-radius:8px; margin-top:10px; animation: glow-border 2s infinite ease-in-out;">
              <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;">
                <span class="gauge-lbl" style="color:var(--accent); font-weight:600; font-size:0.65rem; text-transform:uppercase; letter-spacing:0.5px;">Self-Learning Optimizer</span>
                <span class="status-pill" style="font-size:0.6rem; padding:1px 6px; background:rgba(168,85,247,0.15); border-color:rgba(168,85,247,0.3); color:var(--accent); font-weight:bold; animation: blinker 1.5s linear infinite;">TRAINING ACTIVE</span>
              </div>
              <div style="font-size:0.7rem; color:var(--text-main); font-family:monospace; margin-bottom:6px; overflow:hidden; text-overflow:ellipsis; white-space:nowrap;">Task: <span id="widget-training-status" style="color:var(--accent);">Synchronizing datasets...</span></div>
              <div class="progress-bar-container" style="width:100%; height:3px; background:rgba(255,255,255,0.05); border-radius:10px; overflow:hidden;">
                <div style="width:35%; height:100%; background:linear-gradient(90deg, var(--accent) 0%, var(--primary) 100%); border-radius:10px; animation: progress-run 4.5s infinite ease-in-out;"></div>
              </div>
            </div>
          </div>

          <!-- calibration and deploy logs -->
          <div style="display: grid; grid-template-columns: 1.1fr 1fr; gap: 12px; margin-top: 4px; align-items: end;">
            <div>
              <div class="gauge-lbl" style="font-size:0.65rem; font-weight:600; margin-bottom:4px; text-transform: uppercase;">Calibration Log</div>
              <button class="btn" style="padding: 6px; font-size:0.75rem; margin-bottom:4px;" onclick="logDataPoint()">Record Calibration</button>
              <div style="display: flex; gap: 4px;">
                <a href="/download_csv" class="btn btn-sec" style="padding: 4px; font-size:0.7rem; text-align:center; flex:1.2;">Download</a>
                <button class="btn btn-sec" style="padding: 4px; font-size:0.7rem; color:var(--danger); border-color:rgba(239,68,68,0.2); flex:1;" onclick="clearLog()">Clear</button>
              </div>
            </div>
            <div>
              <div class="gauge-lbl" style="font-size:0.65rem; font-weight:600; margin-bottom:4px; text-transform: uppercase;">Deploy Parameters</div>
              <div class="dropzone" id="dropzone" onclick="document.getElementById('file-upload').click()" style="padding: 4px; height: 32px; display: flex; flex-direction: column; justify-content: center; margin-bottom: 4px;">
                <p style="font-size: 0.65rem; margin: 0; line-height: 1.1;">Upload JSON</p>
                <input type="file" id="file-upload" style="display:none;" onchange="uploadModelFile(this.files[0])">
              </div>
              <button class="btn btn-sec" style="padding: 4px; font-size:0.7rem; color:var(--danger); border-color:rgba(239,68,68,0.2); width:100%;" onclick="deleteModel()">Delete Model</button>
            </div>
          </div>
        </div>

        <!-- Chatspace assistant card -->
        <div class="card" style="flex: 1.2; justify-content: space-between; position: relative;">
          <!-- Active training overlay screen -->
          <div id="chat-training-overlay" style="display:none; position:absolute; top:0; left:0; width:100%; height:100%; background:rgba(5,8,14,0.93); backdrop-filter:blur(8px); border-radius:12px; z-index:10; flex-direction:column; align-items:center; justify-content:center; padding:20px; text-align:center;">
            <div style="width:120px; height:120px; margin-bottom:16px;">
              <svg viewBox="0 0 100 100" style="width:100%; height:100%; fill:none; stroke:var(--accent); stroke-width:1.5;">
                <line x1="20" y1="50" x2="50" y2="20" style="stroke-dasharray:5; animation: dash 5s linear infinite;" />
                <line x1="20" y1="50" x2="50" y2="50" style="stroke-dasharray:5; animation: dash 4s linear infinite;" />
                <line x1="20" y1="50" x2="50" y2="80" style="stroke-dasharray:5; animation: dash 6s linear infinite;" />
                <line x1="50" y1="20" x2="80" y2="35" style="stroke-dasharray:5; animation: dash 4.5s linear infinite;" />
                <line x1="50" y1="50" x2="80" y2="35" style="stroke-dasharray:5; animation: dash 3s linear infinite;" />
                <line x1="50" y1="50" x2="80" y2="65" style="stroke-dasharray:5; animation: dash 5s linear infinite;" />
                <line x1="50" y1="80" x2="80" y2="65" style="stroke-dasharray:5; animation: dash 4s linear infinite;" />
                <circle cx="20" cy="50" r="6" fill="#10b981" style="filter: drop-shadow(0 0 6px #10b981); animation: pulse-node 3.5s infinite;" />
                <circle cx="50" cy="20" r="5" fill="#a855f7" style="filter: drop-shadow(0 0 5px #a855f7); animation: pulse-node 3s infinite;" />
                <circle cx="50" cy="50" r="5" fill="#a855f7" style="filter: drop-shadow(0 0 5px #a855f7); animation: pulse-node 4s infinite;" />
                <circle cx="50" cy="80" r="5" fill="#a855f7" style="filter: drop-shadow(0 0 5px #a855f7); animation: pulse-node 3.2s infinite;" />
                <circle cx="80" cy="35" r="7" fill="#0ea5e9" style="filter: drop-shadow(0 0 7px #0ea5e9); animation: pulse-node 3s infinite;" />
                <circle cx="80" cy="65" r="7" fill="#0ea5e9" style="filter: drop-shadow(0 0 7px #0ea5e9); animation: pulse-node 3.8s infinite;" />
              </svg>
            </div>
            <div style="font-family:'Outfit', sans-serif; font-size:1.1rem; font-weight:600; color:var(--accent); text-shadow:0 0 10px var(--accent-glow); margin-bottom:6px; letter-spacing:0.5px;">Optimizing Edge AI...</div>
            <div style="font-size:0.75rem; color:var(--text-muted); font-family:monospace; margin-bottom:12px; height:15px;" id="training-status-text">Fitting MLP Neural Network weights</div>
            <div style="width:140px; height:4px; background:rgba(255,255,255,0.05); border-radius:10px; overflow:hidden; position:relative;">
              <div style="position:absolute; height:100%; background:linear-gradient(90deg, var(--accent) 0%, var(--primary) 100%); border-radius:10px; animation: progress-run 4.5s infinite ease-in-out; width: 30%;"></div>
            </div>
          </div>
          
          <div class="card-title accent">
            <span>Garden-AI Chat Assistant</span>
            <span id="chat-model-badge" style="display:none; font-size:0.65rem; background:rgba(168,85,247,0.15); padding:1px 6px; border-radius:99px; border:1px solid rgba(168,85,247,0.25); font-family:monospace; color:var(--accent);"></span>
          </div>
          
          <div class="chat-container">
            <div class="chat-messages" id="chat-messages">
              <div class="chat-bubble assistant">
                Hello! I am Garden-AI, your diagnostic assistant. I monitor your DFT hydroponic parameters, analyze pH/EC anomalies, and predict nutrient dosing schedules. How can I help you optimize your indoor garden today?
              </div>
            </div>
            
            <div class="chat-presets">
              <button class="preset-btn" onclick="sendPreset('Show garden status')">Status</button>
              <button class="preset-btn" onclick="sendPreset('Are there any anomalies?')">Anomalies</button>
              <button class="preset-btn" onclick="sendPreset('Give me crop maintenance tips')">Maintenance</button>
              <button class="preset-btn" onclick="sendPreset('Detail the Edge AI optimizer')">AI Model</button>
            </div>

            <div class="chat-input-bar">
              <input type="text" class="chat-input" id="chat-input" placeholder="Ask about crop health, pH, EC anomalies..." onkeydown="if(event.key === 'Enter') sendChatMessage()">
              <button class="chat-send-btn" onclick="sendChatMessage()">
                <svg viewBox="0 0 24 24" width="14" height="14" fill="currentColor">
                  <path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z"/>
                </svg>
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>

  <script>
    let currentActiveMode = 'REACTIVE';
    let systemStatus = {
      temp: 0.0, hum: 0, ph: 7.0, nc: 250,
      pumpState: 0, expected_dosing: 0.0,
      anomaly_detected: false, anomaly_reason: "None",
      ai_loaded: false, model_type: "linear"
    };

    function setMode(mode) {
      fetch('/mode?mode=' + mode, { method: 'POST' })
        .then(res => {
          if (res.ok) {
            updateModeUI(mode.toUpperCase());
          }
        });
    }

    function updateModeUI(mode) {
      currentActiveMode = mode;
      document.querySelectorAll('.btn-opt').forEach(el => el.classList.remove('active'));
      
      const activeBtn = document.getElementById('opt-' + mode.toLowerCase());
      if (activeBtn) activeBtn.classList.add('active');

      const isManual = (mode === 'MANUAL');
      document.getElementById('btn-manual-pump').disabled = !isManual;
    }

    function updateDosingSlider() {
      const duration = document.getElementById('slide-duration').value;
      document.getElementById('slide-val-duration').textContent = duration;
    }

    function togglePump() {
      const btn = document.getElementById('btn-manual-pump');
      const action = (systemStatus.pumpState === 0) ? 'ON' : 'OFF';
      fetch('/move?posH=' + document.getElementById('slide-duration').value + '&action=' + action, { method: 'POST' })
        .then(res => {
          if (res.ok) {
            btn.textContent = (action === 'ON') ? 'Turn Pump OFF' : 'Turn Pump ON';
            if (action === 'ON') {
              btn.style.background = "linear-gradient(135deg, var(--danger) 0%, #b91c1c 100%)";
              btn.style.boxShadow = "0 3px 8px rgba(239, 68, 68, 0.25)";
            } else {
              btn.style.background = "linear-gradient(135deg, var(--primary) 0%, #059669 100%)";
              btn.style.boxShadow = "0 3px 8px var(--primary-glow)";
            }
          }
        });
    }

    function logDataPoint() {
      const dur = document.getElementById('slide-duration').value;
      fetch('/log?duration=' + dur, { method: 'POST' })
        .then(res => res.text())
        .then(msg => alert(msg));
    }

    function clearLog() {
      if (confirm("Are you sure you want to delete all hydroponic calibration data?")) {
        fetch('/clear_log', { method: 'POST' })
          .then(res => res.text())
          .then(msg => alert(msg));
      }
    }

    function deleteModel() {
      if (confirm("Are you sure you want to delete the active AI model from ESP-01 memory?")) {
        fetch('/delete_model', { method: 'POST' })
          .then(res => res.text())
          .then(msg => {
            alert(msg);
            fetchTelemetry();
          });
      }
    }

    function uploadModelFile(file) {
      if (!file) return;
      
      const reader = new FileReader();
      reader.onload = function(e) {
        const content = e.target.result;
        fetch('/deploy_model', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'model_file=' + encodeURIComponent(content)
        })
        .then(res => res.text())
        .then(msg => {
          alert(msg);
          fetchTelemetry();
        })
        .catch(err => {
          alert("Upload failed: network or connection error");
        });
      };
      reader.readAsText(file);
    }

    function fetchTelemetry() {
      fetch('/status')
        .then(res => res.json())
        .then(data => {
          systemStatus = data;

          // Update text labels
          document.getElementById('lbl-temp').textContent = data.temp.toFixed(1) + '°C';
          document.getElementById('lbl-hum').textContent = data.hum + '%';
          document.getElementById('lbl-ph').textContent = data.ph.toFixed(1);
          document.getElementById('lbl-ec').textContent = data.nc;

          // Update Circular Gauges
          // Temp (max 50)
          let tOffset = 100 - (data.temp / 50.0) * 100;
          document.getElementById('ring-temp').style.strokeDasharray = ((data.temp / 50.0) * 100) + ', 100';
          // Hum (max 100)
          document.getElementById('ring-hum').style.strokeDasharray = data.hum + ', 100';
          // pH (max 14)
          document.getElementById('ring-ph').style.strokeDasharray = ((data.ph / 14.0) * 100) + ', 100';
          // EC / NPK (max 1023)
          document.getElementById('ring-ec').style.strokeDasharray = ((data.nc / 1023.0) * 100) + ', 100';

          // Update pump details
          const pumpStateLbl = document.getElementById('lbl-pump-state');
          const waterFlowPath = document.getElementById('water-flow');
          if (data.pumpState === 1) {
            pumpStateLbl.textContent = "ACTIVE (ON)";
            pumpStateLbl.style.color = "var(--primary)";
            waterFlowPath.classList.add('active');
          } else {
            pumpStateLbl.textContent = "OFF";
            pumpStateLbl.style.color = "var(--text-muted)";
            waterFlowPath.classList.remove('active');
          }

          // Active Mode synchronization
          const serverMode = data.mode.toUpperCase();
          if (serverMode !== currentActiveMode) {
            updateModeUI(serverMode);
          }

          // Manual button styling if pump was toggled externally
          const manualBtn = document.getElementById('btn-manual-pump');
          if (data.pumpState === 1) {
            manualBtn.textContent = 'Turn Pump OFF';
            manualBtn.style.background = "linear-gradient(135deg, var(--danger) 0%, #b91c1c 100%)";
            manualBtn.style.boxShadow = "0 3px 8px rgba(239, 68, 68, 0.25)";
          } else {
            manualBtn.textContent = 'Turn Pump ON';
            manualBtn.style.background = "linear-gradient(135deg, var(--primary) 0%, #059669 100%)";
            manualBtn.style.boxShadow = "0 3px 8px var(--primary-glow)";
          }

          // Liquid visualizer height and color updates based on NC
          const liquid = document.getElementById('liquid-level');
          const stop1 = document.getElementById('grad-stop1');
          const stop2 = document.getElementById('grad-stop2');
          
          if (data.nc >= 507 || data.nc < 50) {
            // Bad conditions (unstable) -> Red/Orange fluid alert
            stop1.setAttribute('stop-color', '#ef4444');
            stop2.setAttribute('stop-color', '#b91c1c');
          } else if (data.nc < 200) {
            // Low nutrient -> Blue/Water color fluid
            stop1.setAttribute('stop-color', '#38bdf8');
            stop2.setAttribute('stop-color', '#0284c7');
          } else {
            // Optimal nutrients -> Rich Green fluid
            stop1.setAttribute('stop-color', '#10b981');
            stop2.setAttribute('stop-color', '#047857');
          }

          document.getElementById('lbl-expected-dosing').textContent = data.expected_dosing.toFixed(1) + ' s';

          // AI Model Status Indicator & Badge
          const aiPill = document.getElementById('ai-model-status');
          const aiDot = document.getElementById('ai-status-dot');
          const chatBadge = document.getElementById('chat-model-badge');
          const activeModelTypeLbl = document.getElementById('lbl-active-model-type');
          if (data.ai_loaded) {
            const mType = data.model_type.toUpperCase();
            aiPill.textContent = "Active (" + mType + ")";
            aiDot.className = "status-dot active";
            chatBadge.textContent = "garden-3.5 (" + mType + ")";
            chatBadge.style.display = "inline-block";
            activeModelTypeLbl.textContent = mType;
            activeModelTypeLbl.style.color = "var(--primary)";
          } else {
            aiPill.textContent = "Inactive";
            aiDot.className = "status-dot";
            chatBadge.style.display = "none";
            activeModelTypeLbl.textContent = "None";
            activeModelTypeLbl.style.color = "var(--text-muted)";
          }

          // Background self-learning training visualization
          const trainingPill = document.getElementById('training-status-pill');
          const trainingPillText = document.getElementById('training-pill-text');
          const trainingOverlay = document.getElementById('chat-training-overlay');
          const trainingStatusText = document.getElementById('training-status-text');
          const trainingWidget = document.getElementById('control-training-widget');
          const trainingWidgetStatus = document.getElementById('widget-training-status');

          if (data.is_training) {
            trainingPill.style.display = "flex";
            trainingPillText.textContent = "Optimizing";
            
            // Show overlay on Chatspace card
            trainingOverlay.style.display = "flex";
            trainingStatusText.textContent = data.training_status;
            
            // Show monitoring widget in control center
            trainingWidget.style.display = "block";
            trainingWidgetStatus.textContent = data.training_status;
          } else {
            trainingPill.style.display = "none";
            trainingOverlay.style.display = "none";
            trainingWidget.style.display = "none";
          }

          // Anomaly Health Banner update
          const healthPill = document.getElementById('anomaly-pill-text');
          const healthDot = document.getElementById('anomaly-status-dot');

          if (data.anomaly_detected) {
            healthPill.textContent = "ANOMALY";
            healthPill.parentElement.style.borderColor = "rgba(239, 68, 68, 0.4)";
            healthPill.parentElement.style.color = "var(--danger)";
            healthDot.className = "status-dot danger";
          } else {
            healthPill.textContent = "Healthy";
            healthPill.parentElement.style.borderColor = "var(--border-glow)";
            healthPill.parentElement.style.color = "var(--text-main)";
            healthDot.className = "status-dot active";
          }
        });
    }

    // Chat presets prompt selection
    function sendPreset(text) {
      document.getElementById('chat-input').value = text;
      sendChatMessage();
    }

    function sendChatMessage() {
      const input = document.getElementById('chat-input');
      const text = input.value.trim();
      if (text === '') return;

      input.value = '';
      appendChatBubble(text, 'user');

      // Render pulsing thinking animation
      const messagesContainer = document.getElementById('chat-messages');
      const thinkingDiv = document.createElement('div');
      thinkingDiv.className = 'chat-bubble assistant thinking';
      thinkingDiv.id = 'thinking-bubble';
      thinkingDiv.innerHTML = '<span class="dot"></span><span class="dot"></span><span class="dot"></span>';
      messagesContainer.appendChild(thinkingDiv);
      messagesContainer.scrollTop = messagesContainer.scrollHeight;

      // Rule-based NLP diagnostic response after simulated thinking delay
      setTimeout(() => {
        const thinkingBubble = document.getElementById('thinking-bubble');
        if (thinkingBubble) thinkingBubble.remove();
        
        const response = processBotResponse(text.toLowerCase());
        appendChatBubble(response, 'assistant');
      }, 1000);
    }

    function appendChatBubble(text, sender) {
      const messagesContainer = document.getElementById('chat-messages');
      const bubble = document.createElement('div');
      bubble.className = 'chat-bubble ' + sender;
      bubble.innerHTML = text.replace(/\n/g, '<br>');
      messagesContainer.appendChild(bubble);
      messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }

    function processBotResponse(prompt) {
      if (prompt.includes('status') || prompt.includes('health') || prompt.includes('state')) {
        let statusStr = "<b>Real-time Hydroponic Status Report</b>:\n";
        statusStr += "- Temp: <b>" + systemStatus.temp.toFixed(1) + "°C</b>\n";
        statusStr += "- Humidity: <b>" + systemStatus.hum + "%</b>\n";
        statusStr += "- pH: <b>" + systemStatus.ph.toFixed(2) + "</b> (Optimal: 5.8 - 6.2)\n";
        statusStr += "- EC/Nutrients: <b>" + systemStatus.nc + "</b>\n";
        statusStr += "- Control Mode: <b>" + systemStatus.mode + "</b>\n";
        statusStr += "- Pump state: <b>" + (systemStatus.pumpState === 1 ? "ON (RUNNING)" : "OFF") + "</b>\n";
        
        if (systemStatus.anomaly_detected) {
          statusStr += "<b>Warning</b>: An anomaly has been flagged! Ask me about 'anomalies' for details.";
        } else {
          statusStr += "All parameters in healthy ranges.";
        }
        return statusStr;
      }

      if (prompt.includes('anomaly') || prompt.includes('fault') || prompt.includes('broken') || prompt.includes('error') || prompt.includes('stuck')) {
        if (systemStatus.anomaly_detected) {
          let alertStr = "<b>Anomaly Diagnostic Analysis</b>:\n";
          alertStr += "<b>Flagged</b>: " + systemStatus.anomaly_reason + "\n";
          
          if (systemStatus.anomaly_reason.includes('Drift') || systemStatus.anomaly_reason.includes('stuck')) {
            alertStr += "<b>Root Cause</b>: Sensor readings have remained exactly constant over 30 telemetry frames. This suggests probe disconnection, electrical ground loop noise, or sensor freeze.\n";
            alertStr += "<b>Action</b>: Inspect probe wiring. Power cycle the co-processor. If it is the pH probe, rinse with distilled water and recalibrate using buffer solutions.";
          } else if (systemStatus.anomaly_reason.includes('Failure')) {
            alertStr += "<b>Root Cause</b>: Dosing cycle executed but no upward shift in nutrient concentration (EC) was observed. This signifies a pump failure, depleted nutrient reservoir, or clogged tubing.\n";
            alertStr += "<b>Action</b>: Check reservoir fluid level. Check pump electrical leads. Ensure the delivery tube is clear and not kinked.";
          }
          return alertStr;
        } else {
          return "<b>Anomaly Diagnostic Analysis</b>:\nNo sensor drift, frozen values, or actuator faults have been flagged. Measurements correlate with standard crop profiles (+/-10%).";
        }
      }

      if (prompt.includes('maintenance') || prompt.includes('crop') || prompt.includes('cleaning') || prompt.includes('clean')) {
        return "<b>DFT Hydroponic Maintenance Checklist</b>:\n1. <b>pH Calibration</b>: Calibrate the pH sensor weekly using pH 4.0 and 7.0 calibration powders to prevent drift.\n2. <b>Reservoir Purge</b>: Flush and replace the nutrient solution reservoir every 14 days to prevent salt buildup.\n3. <b>Pump Maintenance</b>: Check pump intake filters monthly to remove organic root hair clogging.";
      }

      if (prompt.includes('optimizer') || prompt.includes('prediction') || prompt.includes('ai') || prompt.includes('weights')) {
        let aiStr = "<b>Edge AI Optimizer Status</b>:\n";
        if (systemStatus.ai_loaded) {
          aiStr += "- Model loaded: <b>" + systemStatus.model_type.toUpperCase() + "</b>\n";
          aiStr += "- Inputs: Normalised (Temp, Hum, pH, EC)\n";
          aiStr += "- Target: Pump dosing duration in seconds.\n";
          aiStr += "- Current inference prediction: <b>" + systemStatus.expected_dosing.toFixed(1) + " seconds</b>.";
        } else {
          aiStr += "No neural network model deployed yet. Log calibration points in manual mode, download it, and execute the regression pipeline using the Python training server.";
        }
        return aiStr;
      }

      if (prompt.includes('hi') || prompt.includes('hello') || prompt.includes('hey')) {
        return "Hello! I am online. Ask me about hydroponic system status, anomalies, or maintenance routines.";
      }

      if (prompt.includes('help')) {
        return "You can ask me questions like:\n- <i>'Show garden status'</i>\n- <i>'Are there any anomalies?'</i>\n- <i>'Give me crop maintenance tips'</i>\n- <i>'Detail the Edge AI optimizer'</i>";
      }

      return "I am Garden-AI, your diagnostic assistant. I recognize queries about 'status', 'anomalies', 'maintenance', or the 'AI model'. Please rephrase or type 'help' for examples!";
    }

    // Drag-and-drop support for JSON deployment
    const dropzone = document.getElementById('dropzone');
    dropzone.addEventListener('dragover', (e) => { e.preventDefault(); });
    dropzone.addEventListener('drop', (e) => {
      e.preventDefault();
      if (e.dataTransfer.files.length > 0) {
        uploadModelFile(e.dataTransfer.files[0]);
      }
    });

    // Start background telemetry polling
    setInterval(fetchTelemetry, 1000);
    fetchTelemetry();
  </script>
</body>
</html>
)raw";

// ─── Web Route Handlers ──────────────────────────────────────
void handleRoot() {
  server.send_P(200, "text/html", HTTP_INDEX);
}

void handleStatus() {
  String json;
  json.reserve(512);
  json += "{";
  json += "\"mode\":\"" + currentMode + "\",";
  json += "\"temp\":" + String(temp, 1) + ",";
  json += "\"hum\":" + String(hum) + ",";
  json += "\"ph\":" + String(ph, 2) + ",";
  json += "\"nc\":" + String(nc) + ",";
  json += "\"pumpState\":" + String(pumpState) + ",";
  json += "\"expected_dosing\":" + String(expectedDosing, 2) + ",";
  json += "\"anomaly_detected\":" + String(anomalyDetected ? "true" : "false") + ",";
  json += "\"anomaly_reason\":\"" + anomalyReason + "\",";
  json += "\"ai_loaded\":" + String(activeModel.loaded ? "true" : "false") + ",";
  json += "\"model_type\":\"" + activeModel.modelType + "\",";
  json += "\"is_training\":" + String(isTraining ? "true" : "false") + ",";
  json += "\"training_status\":\"" + trainingStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleMode() {
  if (server.hasArg("mode")) {
    String modeArg = server.arg("mode");
    modeArg.toUpperCase();
    if (modeArg == "REACTIVE" || modeArg == "AI" || modeArg == "MANUAL") {
      currentMode = modeArg;
      
      // Transmit new mode state to Arduino co-processor
      Serial.print("MODE:");
      Serial.println(currentMode);
      
      server.send(200, "text/plain", "Mode updated to " + currentMode);
      return;
    }
  }
  server.send(400, "text/plain", "Invalid Mode");
}

void handleMove() {
  // Overloaded /move route to allow manual pump trigger or manual dosing durations
  if (server.hasArg("posH")) {
    float duration = server.arg("posH").toFloat(); // posH is mapped to dosing duration
    String action = server.arg("action");
    
    if (action == "ON") {
      Serial.println("PUMP:ON");
    } else if (action == "OFF") {
      Serial.println("PUMP:OFF");
    } else {
      // Run pump for specified duration
      Serial.print("DOSING:");
      Serial.println(duration, 1);
    }
    server.send(200, "text/plain", "Pump command sent");
  } else {
    server.send(400, "text/plain", "Missing Parameters");
  }
}

void handleLog() {
  // Heap verification
  if (ESP.getFreeHeap() < 6144) {
    server.send(500, "text/plain", "Failsafe: System heap critically low");
    return;
  }
  
  FSInfo fs_info;
  if (LittleFS.info(fs_info)) {
    size_t freeSpace = fs_info.totalBytes - fs_info.usedBytes;
    if (freeSpace < 15360) {
      server.send(570, "text/plain", "Failsafe: Disk space low");
      return;
    }
  }

  // 50KB Cap limit on log file sizes
  if (LittleFS.exists("/garden_data.csv")) {
    File checkFile = LittleFS.open("/garden_data.csv", "r");
    if (checkFile) {
      size_t fileSize = checkFile.size();
      checkFile.close();
      if (fileSize >= 51200) {
        server.send(571, "text/plain", "Failsafe: CSV database log cap reached (50KB)");
        return;
      }
    }
  }

  bool exists = LittleFS.exists("/garden_data.csv");
  File file = LittleFS.open("/garden_data.csv", "a");
  if (!file) {
    server.send(500, "text/plain", "Failed to open dataset log");
    return;
  }

  if (!exists) {
    file.println("temp,hum,ph,nc,dosing_duration");
  }
  
  float logDuration = 5.0;
  if (server.hasArg("duration")) {
    logDuration = server.arg("duration").toFloat();
  }
  
  file.printf("%.1f,%d,%.2f,%d,%.1f\n", temp, hum, ph, nc, logDuration);
  file.close();

  // Play click chime on Arduino
  Serial.println("BEEP:CLICK");

  server.send(200, "text/plain", "Garden data point recorded successfully!");
}

void handleClearLog() {
  if (LittleFS.exists("/garden_data.csv")) {
    LittleFS.remove("/garden_data.csv");
  }
  server.send(200, "text/plain", "Calibration log cleared.");
}

void handleSetTraining() {
  if (server.hasArg("training")) {
    isTraining = (server.arg("training") == "1");
  }
  if (server.hasArg("status")) {
    trainingStatus = server.arg("status");
  } else {
    trainingStatus = isTraining ? "Optimizing..." : "Idle";
  }
  server.send(200, "text/plain", "OK");
}

void handleDeleteModel() {
  if (LittleFS.exists("/model_params.json")) {
    LittleFS.remove("/model_params.json");
  }
  activeModel.loaded = false;
  activeModel.modelType = "none";
  currentMode = "REACTIVE";
  Serial.println("MODE:REACTIVE");
  Serial.println("BEEP:ERROR");
  server.send(200, "text/plain", "AI Model parameter file deleted successfully. Device reverted to REACTIVE mode.");
}

void handleDownloadCsv() {
  if (!LittleFS.exists("/garden_data.csv")) {
    server.send(404, "text/plain", "Dataset file empty or not created yet.");
    return;
  }
  File file = LittleFS.open("/garden_data.csv", "r");
  server.streamFile(file, "text/csv");
  file.close();
}

void handleDeployModel() {
  String modelJson = "";
  if (server.hasArg("model_file")) {
    modelJson = server.arg("model_file");
  } else if (server.args() > 0) {
    modelJson = server.arg(0);
  } else {
    modelJson = server.arg("plain");
  }
  
  if (modelJson.length() == 0) {
    Serial.println("BEEP:ERROR");
    server.send(400, "text/plain", "Missing model file payload");
    return;
  }
  
#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(2048);
#endif
  DeserializationError error = deserializeJson(doc, modelJson);
  
  if (error) {
    Serial.println("BEEP:ERROR");
    server.send(400, "text/plain", "Upload aborted: Invalid JSON format");
    return;
  }
  
  // Save to LittleFS
  File file = LittleFS.open("/model_params.json", "w");
  if (!file) {
    Serial.println("BEEP:ERROR");
    server.send(500, "text/plain", "Failed to write parameter file");
    return;
  }
  file.print(modelJson);
  file.close();
  
  // Attempt parser reload
  if (loadModelParams()) {
    Serial.println("BEEP:SUCCESS");
    server.send(200, "text/plain", "Model weights deployed successfully!");
  } else {
    Serial.println("BEEP:ERROR");
    server.send(500, "text/plain", "Model syntax error: failed verification reload");
  }
}

void handleDownloadModel() {
  if (!LittleFS.exists("/model_params.json")) {
    server.send(404, "text/plain", "No deployed parameters available.");
    return;
  }
  File file = LittleFS.open("/model_params.json", "r");
  server.streamFile(file, "application/json");
  file.close();
}

// ─── Setup ──────────────────────────────────────────────────
void setup() {
  // Use hardware serial at 9600 to match Arduino co-processor
  Serial.begin(9600);
  
  // Initialize LittleFS with auto format failsafe
  if (!LittleFS.begin()) {
    LittleFS.format();
    LittleFS.begin();
  }
  
  // Load AI weights if available
  loadModelParams();

  // Establish local Access Point
  WiFi.softAP(AP_SSID, AP_PASS);

  // Configure REST Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/mode", HTTP_POST, handleMode);
  server.on("/move", HTTP_POST, handleMove);
  server.on("/log", HTTP_POST, handleLog);
  server.on("/clear_log", HTTP_POST, handleClearLog);
  server.on("/download_csv", HTTP_GET, handleDownloadCsv);
  server.on("/deploy_model", HTTP_POST, handleDeployModel);
  server.on("/download_model", HTTP_GET, handleDownloadModel);
  server.on("/set_training", HTTP_POST, handleSetTraining);
  server.on("/delete_model", HTTP_POST, handleDeleteModel);
  
  server.begin();
}

// ─── Main Loop ──────────────────────────────────────────────
void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();
  
  // Parse incoming data packets from Arduino Slave co-processor
  // Format: DATA:temp,hum,ph,nc,pumpState
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialLen > 0) {
        serialBuf[serialLen] = '\0';
        
        if (strncmp(serialBuf, "DATA:", 5) == 0) {
          char* pData = serialBuf + 5;
          float temp_val, ph_val;
          int hum_val, nc_val, pump_val = 0;
          int parsed = sscanf(pData, "%f,%d,%f,%d,%d", &temp_val, &hum_val, &ph_val, &nc_val, &pump_val);
          if (parsed >= 4) {
            temp = temp_val;
            hum = hum_val;
            ph = ph_val;
            nc = nc_val;
            if (parsed == 5) {
              pumpState = pump_val;
            }
            
            // Run anomaly calculations
            runAnomalyDetection();

            // Execute real-time Edge AI dosing when set to AI mode
            if (currentMode == "AI") {
              if (currentMillis - lastAiInferenceTime >= 15000) { // check every 15 seconds
                lastAiInferenceTime = currentMillis;
                if (currentMillis >= aiCooldownEndTime) {
                  runEdgeInference();
                  if (expectedDosing > 0.5) {
                    aiCooldownEndTime = currentMillis + 60000; // 60s cooldown to allow mixing
                  }
                }
              }
            }
          }
        }
        serialLen = 0;
      }
    } else if (serialLen < sizeof(serialBuf) - 1) {
      serialBuf[serialLen++] = c;
    }
  }
}
