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
#include "index_html.h"

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
