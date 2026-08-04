import os
import json
import time
import requests
import threading
from io import StringIO
import numpy as np
import pandas as pd
from flask import Flask, request, jsonify, render_template_string, send_file, redirect, url_for, Response
from sklearn.linear_model import LinearRegression
from sklearn.neural_network import MLPRegressor

app = Flask(__name__)

# Cache states for auto-training
last_seen_rows = 0
last_r2_score = -1.0

# Global state for training data (for the manual training demo)
global_training_data = None

# ═══════════════════════════════════════════════════════════
#  PREMIUM GLASSMORPHIC HTML TEMPLATE (UPLOAD)
# ═══════════════════════════════════════════════════════════
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Garden-AI Training Studio</title>
  <style>
    :root {
      --bg-color: #04080e;
      --panel-bg: rgba(10, 20, 15, 0.75);
      --border-color: rgba(16, 185, 129, 0.2);
      --glow-color: #10b981;
      --text-color: #f8fafc;
      --text-muted: #64748b;
      --accent-color: #a855f7;
    }
    body {
      background-color: var(--bg-color);
      color: var(--text-color);
      font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
      margin: 0;
      padding: 30px 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
      background-image: 
        radial-gradient(circle at 10% 10%, rgba(16, 185, 129, 0.08) 0%, transparent 40%),
        radial-gradient(circle at 90% 90%, rgba(168, 85, 247, 0.06) 0%, transparent 40%);
    }
    .container {
      width: 100%;
      max-width: 750px;
    }
    header {
      text-align: center;
      margin-bottom: 40px;
    }
    h1 {
      font-size: 2.8rem;
      margin: 0;
      background: linear-gradient(135deg, #10b981 0%, #a855f7 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      filter: drop-shadow(0 0 15px rgba(16, 185, 129, 0.25));
    }
    .subtitle {
      color: var(--text-muted);
      margin-top: 8px;
      font-size: 1.1rem;
    }
    .panel {
      background: var(--panel-bg);
      border: 1px solid var(--border-color);
      border-radius: 20px;
      padding: 32px;
      margin-bottom: 28px;
      backdrop-filter: blur(16px);
      box-shadow: 0 12px 40px 0 rgba(0, 0, 0, 0.5);
    }
    h2 {
      margin-top: 0;
      font-size: 1.4rem;
      color: var(--glow-color);
      border-bottom: 1px solid var(--border-color);
      padding-bottom: 12px;
      padding-top: 6px;
    }
    .form-group {
      margin-bottom: 24px;
    }
    label {
      display: block;
      font-weight: 600;
      margin-bottom: 8px;
      color: var(--text-color);
    }
    select {
      width: 100%;
      background-color: #050d0a;
      border: 1px solid var(--border-color);
      border-radius: 8px;
      color: white;
      padding: 12px;
      font-size: 1rem;
      outline: none;
      cursor: pointer;
    }
    .btn {
      background: linear-gradient(135deg, #10b981 0%, #a855f7 100%);
      border: none;
      color: white;
      padding: 14px 28px;
      border-radius: 8px;
      cursor: pointer;
      font-weight: bold;
      transition: all 0.2s ease;
      display: inline-block;
      text-decoration: none;
      box-shadow: 0 0 15px rgba(16, 185, 129, 0.25);
      text-align: center;
      width: 100%;
      box-sizing: border-box;
      font-size: 1rem;
    }
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 0 25px rgba(16, 185, 129, 0.45);
    }
    .file-input {
      display: none;
    }
    .file-label {
      border: 2px dashed var(--border-color);
      border-radius: 12px;
      padding: 30px;
      display: flex;
      flex-direction: column;
      align-items: center;
      cursor: pointer;
      transition: all 0.2s ease;
      background: rgba(10, 20, 15, 0.4);
    }
    .file-label:hover {
      border-color: var(--glow-color);
      background: rgba(10, 20, 15, 0.6);
    }
    .file-label span {
      margin-top: 12px;
      color: var(--text-muted);
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Garden-AI Studio</h1>
      <p class="subtitle">Edge AI Nutrient Optimization Calibration Engine</p>
    </header>

    <div class="panel">
      <h2>Upload Dataset & Select AI Model</h2>
      <form action="/train" method="POST" enctype="multipart/form-data">
        <div class="form-group">
          <label class="file-label">
            <svg style="width: 54px; height: 54px; fill: var(--text-muted);" viewBox="0 0 24 24">
              <path d="M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM14 13v4h-4v-4H7l5-5 5 5h-3z"/>
            </svg>
            <span id="file-name-display">Drop garden_data.csv here or click to upload</span>
            <input type="file" name="csv_file" class="file-input" id="csv_file" required onchange="updateFileName()">
          </label>
        </div>

        <div class="form-group">
          <label for="model_type">Regression Algorithm</label>
          <select name="model_type" id="model_type">
            <option value="linear">Multivariate Linear Regression</option>
            <option value="mlp">Multi-layer Perceptron (MLP) Neural Network</option>
          </select>
        </div>

        <button type="submit" class="btn">Execute Machine Learning Pipeline</button>
      </form>
    </div>

    <div style="text-align: center; color: var(--text-muted); font-size: 0.85rem; margin-top: 20px;">
      YSK 2026 Smart Indoor Garden AI | Kisii School
    </div>
  </div>

  <script>
    function updateFileName() {
      const fileInput = document.getElementById('csv_file');
      const nameDisplay = document.getElementById('file-name-display');
      if (fileInput.files.length > 0) {
        nameDisplay.textContent = fileInput.files[0].name;
      }
    }
  </script>
</body>
</html>
"""

# ═══════════════════════════════════════════════════════════
# ═══════════════════════════════════════════════════════════
#  LIVE VISUALIZATION HTML TEMPLATE (ZERO SCROLLBARS 100VH)
# ═══════════════════════════════════════════════════════════
HTML_VIS_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Garden-AI Live Training Studio</title>
  <style>
    :root {
      --bg-color: #04080e;
      --panel-bg: rgba(10, 20, 15, 0.75);
      --border-color: rgba(16, 185, 129, 0.2);
      --glow-color: #10b981;
      --text-color: #f8fafc;
      --text-muted: #64748b;
      --accent-color: #a855f7;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    html, body {
      height: 100vh;
      width: 100vw;
      overflow: hidden;
      background-color: var(--bg-color);
      color: var(--text-color);
      font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
      padding: 10px 14px;
      background-image: 
        radial-gradient(circle at 10% 10%, rgba(16, 185, 129, 0.08) 0%, transparent 40%),
        radial-gradient(circle at 90% 90%, rgba(168, 85, 247, 0.06) 0%, transparent 40%);
    }
    .app-layout {
      display: flex;
      flex-direction: column;
      height: 100%;
      width: 100%;
      max-width: 1400px;
      margin: 0 auto;
      gap: 8px;
    }
    .header-bar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      background: var(--panel-bg);
      border: 1px solid var(--border-color);
      border-radius: 10px;
      padding: 8px 14px;
      gap: 16px;
      flex-shrink: 0;
      backdrop-filter: blur(12px);
    }
    .header-bar h2 {
      font-size: 1.1rem;
      margin: 0;
      white-space: nowrap;
    }
    .top-status {
      flex: 1;
      display: flex;
      flex-direction: column;
      gap: 4px;
    }
    .status-info {
      display: flex;
      justify-content: space-between;
      font-size: 0.85rem;
    }
    .status-msg { font-weight: 600; color: var(--glow-color); }
    .epoch-counter { font-weight: 600; color: var(--accent-color); }
    .progress-container { width: 100%; height: 6px; background: #0f172a; border-radius: 3px; overflow: hidden; }
    .progress-bar { height: 100%; width: 0%; background: linear-gradient(90deg, #10b981, #a855f7); transition: width 0.1s; }

    .dashboard-grid {
      flex: 1;
      display: flex;
      flex-direction: column;
      gap: 8px;
      min-height: 0;
    }
    .grid-row {
      flex: 1;
      display: grid;
      gap: 8px;
      min-height: 0;
    }
    .grid-row.two-cols { grid-template-columns: 1fr 1fr; }
    .grid-row.three-cols { grid-template-columns: 1fr 1fr 1.1fr; }

    .card {
      background: var(--panel-bg);
      border: 1px solid var(--border-color);
      border-radius: 10px;
      padding: 8px 12px;
      display: flex;
      flex-direction: column;
      min-height: 0;
      backdrop-filter: blur(12px);
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.3);
    }
    .card-title {
      font-size: 0.78rem;
      font-weight: 600;
      color: var(--text-muted);
      margin-bottom: 4px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      flex-shrink: 0;
    }
    .canvas-wrapper {
      flex: 1;
      position: relative;
      width: 100%;
      min-height: 0;
    }
    canvas {
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0, 0, 0, 0.3);
      border-radius: 6px;
      border: 1px solid rgba(255,255,255,0.05);
    }

    .metric-card-content {
      flex: 1;
      display: flex;
      align-items: center;
      justify-content: space-around;
      padding: 4px;
    }
    .r2-gauge {
      width: 86px;
      height: 86px;
      border-radius: 50%;
      border: 6px solid #1e293b;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      font-size: 1.15rem;
      font-weight: bold;
      position: relative;
      box-shadow: 0 0 15px rgba(0,0,0,0.4);
      flex-shrink: 0;
    }
    .r2-label { font-size: 0.65rem; color: var(--text-muted); font-weight: normal; margin-top: 1px; }
    .mae-container { text-align: center; }
    .mae-val { font-size: 1.3rem; font-weight: bold; color: var(--text-color); }
    .mae-lbl { font-size: 0.72rem; color: var(--text-muted); }

    .footer-bar {
      background: var(--panel-bg);
      border: 1px solid var(--border-color);
      border-radius: 10px;
      padding: 6px 14px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      flex-shrink: 0;
      height: 44px;
      backdrop-filter: blur(12px);
    }
    .log-ticker {
      flex: 1;
      font-family: monospace;
      font-size: 0.8rem;
      color: #a1a1aa;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    .btn-group { display: flex; align-items: center; gap: 8px; flex-shrink: 0; }
    .btn {
      background: linear-gradient(135deg, #10b981 0%, #a855f7 100%);
      border: none; color: white; padding: 6px 16px; border-radius: 6px;
      cursor: pointer; font-weight: bold; font-size: 0.82rem;
      text-decoration: none; display: inline-block;
      box-shadow: 0 0 10px rgba(16, 185, 129, 0.25);
      white-space: nowrap;
      transition: all 0.2s ease;
    }
    .btn:hover { box-shadow: 0 0 18px rgba(16, 185, 129, 0.45); transform: translateY(-1px); }
    .btn-sec { background: #334155; box-shadow: none; }
    .btn-sec:hover { background: #475569; box-shadow: none; }
  </style>
</head>
<body>
  <div class="app-layout">
    <!-- Top Header & Progress Bar -->
    <div class="header-bar">
      <h2><span style="color:var(--glow-color)">AI Training:</span> {{ model_type | upper }}</h2>
      <div class="top-status">
        <div class="status-info">
          <span class="status-msg" id="status-msg">Initializing training pipeline...</span>
          <span class="epoch-counter" id="epoch-counter">Epoch: 0 / 0</span>
        </div>
        <div class="progress-container">
          <div class="progress-bar" id="progress-bar"></div>
        </div>
      </div>
    </div>
    
    <!-- Dashboard Charts Grid (Flex Fill, Zero Overflow) -->
    <div class="dashboard-grid">
      {% if model_type == 'mlp' %}
      <!-- Row 1: Loss, R²/MAE, Neural Net Architecture -->
      <div class="grid-row three-cols">
        <div class="card">
          <div class="card-title">Loss Curve (MSE)</div>
          <div class="canvas-wrapper"><canvas id="lossCanvas"></canvas></div>
        </div>
        <div class="card">
          <div class="card-title">Model Performance</div>
          <div class="metric-card-content">
            <div class="r2-gauge" id="r2-gauge" style="background: conic-gradient(var(--glow-color) 0%, #1e293b 0%);">
              <span id="r2-val">0.00</span>
              <span class="r2-label">R² Score</span>
            </div>
            <div class="mae-container">
              <div class="mae-val" id="mae-val">0.00</div>
              <div class="mae-lbl">Mean Abs Error</div>
            </div>
          </div>
        </div>
        <div class="card">
          <div class="card-title">Neural Network Architecture</div>
          <div class="canvas-wrapper"><canvas id="networkCanvas"></canvas></div>
        </div>
      </div>

      <!-- Row 2: Actual vs Predicted, Model Weights -->
      <div class="grid-row two-cols">
        <div class="card">
          <div class="card-title">Actual vs Predicted</div>
          <div class="canvas-wrapper"><canvas id="scatterCanvas"></canvas></div>
        </div>
        <div class="card">
          <div class="card-title">Model Weights (Synapses)</div>
          <div class="canvas-wrapper"><canvas id="weightsCanvas"></canvas></div>
        </div>
      </div>

      {% else %}

      <!-- Linear Mode: 2x2 Clean Layout -->
      <!-- Row 1: Loss Curve, R²/MAE Performance -->
      <div class="grid-row two-cols">
        <div class="card">
          <div class="card-title">Loss Curve (MSE)</div>
          <div class="canvas-wrapper"><canvas id="lossCanvas"></canvas></div>
        </div>
        <div class="card">
          <div class="card-title">Model Performance</div>
          <div class="metric-card-content">
            <div class="r2-gauge" id="r2-gauge" style="background: conic-gradient(var(--glow-color) 0%, #1e293b 0%);">
              <span id="r2-val">0.00</span>
              <span class="r2-label">R² Score</span>
            </div>
            <div class="mae-container">
              <div class="mae-val" id="mae-val">0.00</div>
              <div class="mae-lbl">Mean Abs Error</div>
            </div>
          </div>
        </div>
      </div>

      <!-- Row 2: Actual vs Predicted, Model Weights -->
      <div class="grid-row two-cols">
        <div class="card">
          <div class="card-title">Actual vs Predicted</div>
          <div class="canvas-wrapper"><canvas id="scatterCanvas"></canvas></div>
        </div>
        <div class="card">
          <div class="card-title">Model Weights (Feature Weights)</div>
          <div class="canvas-wrapper"><canvas id="weightsCanvas"></canvas></div>
        </div>
      </div>
      {% endif %}
    </div>
    
    <!-- Footer Log Ticker & Action Controls -->
    <div class="footer-bar">
      <div class="log-ticker" id="log-ticker">System ready. Waiting for training events...</div>
      <div class="btn-group" id="download-section" style="display:none;">
        <a href="/download" class="btn">Download model_params.json</a>
        <a href="/" class="btn btn-sec">Return to Studio</a>
      </div>
    </div>
  </div>

  <script>
    const lossCanvas = document.getElementById('lossCanvas');
    const scatterCanvas = document.getElementById('scatterCanvas');
    const weightsCanvas = document.getElementById('weightsCanvas');
    const networkCanvas = document.getElementById('networkCanvas');
    
    function resizeAllCanvases() {
      [lossCanvas, scatterCanvas, weightsCanvas, networkCanvas].forEach(c => {
        if(!c) return;
        const parent = c.parentElement;
        if(parent && (parent.clientWidth !== c.width || parent.clientHeight !== c.height)) {
          c.width = parent.clientWidth;
          c.height = parent.clientHeight;
        }
      });
    }
    
    window.addEventListener('resize', resizeAllCanvases);
    setTimeout(resizeAllCanvases, 50);
    
    const lossCtx = lossCanvas ? lossCanvas.getContext('2d') : null;
    const scatterCtx = scatterCanvas ? scatterCanvas.getContext('2d') : null;
    const weightsCtx = weightsCanvas ? weightsCanvas.getContext('2d') : null;
    const netCtx = networkCanvas ? networkCanvas.getContext('2d') : null;
    
    let lossHistory = [];
    
    function logMsg(msg) {
      document.getElementById('log-ticker').innerText = `[${new Date().toLocaleTimeString()}] ${msg}`;
      document.getElementById('status-msg').innerText = msg;
    }
    
    function drawLoss(loss) {
      if(!lossCtx) return;
      lossHistory.push(loss);
      lossCtx.clearRect(0, 0, lossCanvas.width, lossCanvas.height);
      if (lossHistory.length < 2) return;
      
      const maxLoss = Math.max(...lossHistory) * 1.1;
      const stepX = lossCanvas.width / Math.max(10, lossHistory.length);
      
      lossCtx.beginPath();
      lossCtx.strokeStyle = '#a855f7';
      lossCtx.lineWidth = 2.5;
      
      lossHistory.forEach((val, i) => {
        const x = i * stepX;
        const y = lossCanvas.height - (val / maxLoss) * (lossCanvas.height - 10) - 5;
        if(i === 0) lossCtx.moveTo(x, y);
        else lossCtx.lineTo(x, y);
      });
      lossCtx.stroke();
    }
    
    function drawScatter(actuals, preds) {
      if(!scatterCtx || !actuals || !preds) return;
      scatterCtx.clearRect(0, 0, scatterCanvas.width, scatterCanvas.height);
      
      const maxVal = Math.max(...actuals, ...preds, 1);
      const padding = 12;
      const w = scatterCanvas.width - padding*2;
      const h = scatterCanvas.height - padding*2;
      const scaleX = w / maxVal;
      const scaleY = h / maxVal;
      
      // Draw diagonal line of best fit
      scatterCtx.beginPath();
      scatterCtx.strokeStyle = 'rgba(255,255,255,0.2)';
      scatterCtx.setLineDash([4, 4]);
      scatterCtx.moveTo(padding, scatterCanvas.height - padding);
      scatterCtx.lineTo(scatterCanvas.width - padding, padding);
      scatterCtx.stroke();
      scatterCtx.setLineDash([]);
      
      // Draw scatter points
      for(let i=0; i<actuals.length; i++) {
        const x = padding + actuals[i] * scaleX;
        const y = scatterCanvas.height - padding - (preds[i] * scaleY);
        
        scatterCtx.beginPath();
        scatterCtx.arc(x, y, 3.5, 0, Math.PI*2);
        scatterCtx.fillStyle = '#10b981';
        scatterCtx.fill();
      }
    }
    
    function drawWeights(weights) {
      if(!weightsCtx || !weights || weights.length === 0) return;
      weightsCtx.clearRect(0, 0, weightsCanvas.width, weightsCanvas.height);
      
      const maxW = Math.max(...weights.map(Math.abs), 0.1);
      const barH = Math.min(18, (weightsCanvas.height - 15) / weights.length);
      const centerX = weightsCanvas.width / 2;
      
      weightsCtx.beginPath();
      weightsCtx.strokeStyle = 'rgba(255,255,255,0.15)';
      weightsCtx.moveTo(centerX, 0);
      weightsCtx.lineTo(centerX, weightsCanvas.height);
      weightsCtx.stroke();
      
      weights.forEach((w, i) => {
        const y = 6 + i * (barH + 4);
        const wPx = (w / maxW) * (weightsCanvas.width / 2 - 15);
        
        weightsCtx.fillStyle = w > 0 ? '#10b981' : '#f43f5e';
        if(w > 0) {
          weightsCtx.fillRect(centerX, y, wPx, barH);
        } else {
          weightsCtx.fillRect(centerX + wPx, y, -wPx, barH);
        }
      });
    }
    
    function drawNetwork(w1, w2) {
      if(!netCtx || !w1 || !w2) return;
      netCtx.clearRect(0, 0, networkCanvas.width, networkCanvas.height);
      
      const w = networkCanvas.width;
      const h = networkCanvas.height;
      
      const inNodes = 4, hidNodes = 8, outNodes = 1;
      const pad = 12;
      
      function drawLayer(x, count, color) {
        const spacing = (h - pad*2) / Math.max(1, count - 1);
        const nodes = [];
        for(let i=0; i<count; i++) {
          const y = count === 1 ? h/2 : pad + i * spacing;
          nodes.push({x, y});
          netCtx.beginPath();
          netCtx.arc(x, y, 4.5, 0, Math.PI*2);
          netCtx.fillStyle = color;
          netCtx.fill();
        }
        return nodes;
      }
      
      const x1 = pad + 15, x2 = w/2, x3 = w - pad - 15;
      const n1 = drawLayer(x1, inNodes, '#3b82f6');
      const n2 = drawLayer(x2, hidNodes, '#a855f7');
      const n3 = drawLayer(x3, outNodes, '#10b981');
      
      netCtx.lineWidth = 1;
      
      for(let i=0; i<hidNodes; i++) {
        for(let j=0; j<inNodes; j++) {
          const val = w1[i] ? w1[i][j] : 0;
          netCtx.beginPath();
          netCtx.moveTo(n1[j].x, n1[j].y);
          netCtx.lineTo(n2[i].x, n2[i].y);
          const opacity = Math.min(1.0, Math.abs(val) * 2);
          netCtx.strokeStyle = val > 0 ? `rgba(16,185,129,${opacity})` : `rgba(244,63,94,${opacity})`;
          netCtx.stroke();
        }
      }
      
      for(let i=0; i<outNodes; i++) {
        for(let j=0; j<hidNodes; j++) {
          const val = w2[j] || 0;
          netCtx.beginPath();
          netCtx.moveTo(n2[j].x, n2[j].y);
          netCtx.lineTo(n3[i].x, n3[i].y);
          const opacity = Math.min(1.0, Math.abs(val) * 2);
          netCtx.strokeStyle = val > 0 ? `rgba(16,185,129,${opacity})` : `rgba(244,63,94,${opacity})`;
          netCtx.stroke();
        }
      }
    }
    
    const es = new EventSource('/train_events?model_type={{ model_type }}');
    
    es.addEventListener('status', (e) => {
      const data = JSON.parse(e.data);
      logMsg(data.message);
    });
    
    es.addEventListener('epoch', (e) => {
      resizeAllCanvases();
      const data = JSON.parse(e.data);
      
      document.getElementById('epoch-counter').innerText = `Epoch: ${data.epoch} / ${data.total}`;
      document.getElementById('progress-bar').style.width = `${(data.epoch / data.total) * 100}%`;
      
      const r2 = Math.max(0, data.r2);
      document.getElementById('r2-val').innerText = r2.toFixed(3);
      document.getElementById('r2-gauge').style.background = `conic-gradient(var(--glow-color) ${r2 * 100}%, #1e293b 0%)`;
      document.getElementById('mae-val').innerText = data.mae.toFixed(2);
      
      drawLoss(data.loss);
      drawScatter(data.actuals, data.predictions);
      
      if(data.weights && data.weights.length > 0) {
        drawWeights(data.weights);
      } else if (data.w1 && data.w2) {
        drawWeights(data.w2);
        drawNetwork(data.w1, data.w2);
      }
    });
    
    es.addEventListener('complete', (e) => {
      const data = JSON.parse(e.data);
      logMsg("Training complete! Model parameters are ready for deployment.");
      document.getElementById('download-section').style.display = 'flex';
      es.close();
    });
    
    es.onerror = () => {
      es.close();
      logMsg("Training session concluded.");
    };
  </script>
</body>
</html>
"""


# ═══════════════════════════════════════════════════════════
#  CLOSED-LOOP BACKGROUND SELF-TRAINING SYNC LOOP
# ═══════════════════════════════════════════════════════════

def background_sync_loop():
    global last_seen_rows, last_r2_score
    print("Background garden training sync thread started...")
    
    while True:
        try:
            url_csv = "http://192.168.4.1/download_csv"
            response = requests.get(url_csv, timeout=3)
            
            if response.status_code == 200:
                csv_data = response.text
                df = pd.read_csv(StringIO(csv_data))
                current_rows = len(df)
                
                # Auto-train if we have new rows (min 5 calibration points)
                if current_rows > 5 and current_rows != last_seen_rows:
                    print(f"[Sync] New calibration data: {current_rows} rows. Training updated model...")
                    try:
                        # Notify start
                        try:
                            requests.post("http://192.168.4.1/set_training", data={"training": "1", "status": "Synchronizing datasets..."}, timeout=2)
                        except:
                            pass
                        
                        # Min-Max Scaling (normalizing inputs to 0.0 - 1.0)
                        df['n_temp'] = df['temp'] / 50.0
                        df['n_hum'] = df['hum'] / 100.0
                        df['n_ph'] = df['ph'] / 14.0
                        df['n_nc'] = df['nc'] / 1023.0
                        
                        X = df[['n_temp', 'n_hum', 'n_ph', 'n_nc']].values
                        y = df['dosing_duration'].values
                        
                        # Notify training start
                        try:
                            requests.post("http://192.168.4.1/set_training", data={"training": "1", "status": "Fitting MLP network weights..."}, timeout=2)
                        except:
                            pass

                        # Train MLP
                        model = MLPRegressor(
                            hidden_layer_sizes=(8,),
                            activation='relu',
                            solver='lbfgs',
                            max_iter=4000,
                            random_state=42
                        )
                        model.fit(X, y)
                        y_pred = model.predict(X)
                        
                        # Compute R2 score
                        r2 = float(np.clip(1.0 - (np.sum((y - y_pred)**2) / np.sum((y - np.mean(y))**2)), 0.0, 1.0))
                        
                        print(f"[Sync] Model fit successful. R2 = {r2:.4f}")
                        
                        # Auto-deploy if R2 improves over previous best
                        if r2 > last_r2_score:
                            try:
                                requests.post("http://192.168.4.1/set_training", data={"training": "1", "status": "Deploying optimized parameters..."}, timeout=2)
                            except:
                                pass
                                
                            w1 = model.coefs_[0].T.tolist()
                            b1 = model.intercepts_[0].tolist()
                            w2 = model.coefs_[1].T.flatten().tolist()
                            b2 = float(model.intercepts_[1][0])
                            
                            params_dict = {
                                "model_type": "mlp",
                                "w1": w1,
                                "b1": b1,
                                "w2": w2,
                                "b2": b2
                            }
                            
                            # POST to ESP-01
                            url_deploy = "http://192.168.4.1/deploy_model"
                            deploy_res = requests.post(
                                url_deploy, 
                                data={"model_file": json.dumps(params_dict)}, 
                                timeout=5
                            )
                            if deploy_res.status_code == 200:
                                print(f"[Sync] Successfully deployed optimized weights to ESP-01 (R2: {r2:.4f})")
                                last_r2_score = r2
                                
                                # Cache parameters locally
                                temp_path = os.path.join(os.path.dirname(__file__), 'model_params.json')
                                with open(temp_path, 'w') as f:
                                    json.dump(params_dict, f, indent=2)
                            else:
                                print(f"[Sync] Deploy failed with code {deploy_res.status_code}: {deploy_res.text}")
                                
                        last_seen_rows = current_rows
                    finally:
                        # Reset training state on ESP-01
                        try:
                            requests.post("http://192.168.4.1/set_training", data={"training": "0", "status": "Idle"}, timeout=2)
                        except:
                            pass
                    
        except requests.exceptions.RequestException:
            # Standby if ESP-01 AP is offline or not reachable
            pass
        except Exception as e:
            print(f"[Sync Error] {e}")
            
        time.sleep(10)

# ═══════════════════════════════════════════════════════════
#  FLASK ROUTING LOGIC
# ═══════════════════════════════════════════════════════════

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/train', methods=['POST'])
def train():
    global global_training_data
    if 'csv_file' not in request.files:
        return "No file uploaded", 400
    
    file = request.files['csv_file']
    model_type = request.form.get('model_type', 'linear')
    
    if file.filename == '':
        return "Empty file uploaded", 400
    
    try:
        df = pd.read_csv(file)
    except Exception as e:
        return f"Failed to parse CSV file: {e}", 400
    
    required_cols = ['temp', 'hum', 'ph', 'nc', 'dosing_duration']
    for c in required_cols:
        if c not in df.columns:
            return f"Missing required column in CSV: {c}", 400
            
    df['n_temp'] = df['temp'] / 50.0
    df['n_hum'] = df['hum'] / 100.0
    df['n_ph'] = df['ph'] / 14.0
    df['n_nc'] = df['nc'] / 1023.0
    
    X = df[['n_temp', 'n_hum', 'n_ph', 'n_nc']].values
    y = df['dosing_duration'].values
    
    global_training_data = (X, y)
    
    return redirect(url_for('training', model_type=model_type))

@app.route('/training')
def training():
    model_type = request.args.get('model_type', 'linear')
    if global_training_data is None:
        return redirect(url_for('index'))
    return render_template_string(HTML_VIS_TEMPLATE, model_type=model_type)

@app.route('/train_events')
def train_events():
    model_type = request.args.get('model_type', 'linear')
    if global_training_data is None:
        return "No data", 400

    def generate():
        X, y = global_training_data
        
        def get_r2(y_true, y_pred):
            var_y = np.sum((y_true - np.mean(y_true))**2)
            if var_y == 0: return 0.0
            return float(np.clip(1.0 - (np.sum((y_true - y_pred)**2) / var_y), 0.0, 1.0))
            
        def format_sse(event, data):
            return f"event: {event}\ndata: {json.dumps(data)}\n\n"

        yield format_sse("status", {"message": "Initializing with random weights — the AI is starting from scratch...", "phase": "init"})
        
        if model_type == 'linear':
            np.random.seed(42)
            W = np.random.randn(4) * 0.1
            b = np.random.randn() * 0.1
            lr = 0.05
            epochs = 120
            
            for epoch in range(1, epochs + 1):
                idx = np.random.permutation(len(X))
                X_shuf = X[idx]
                y_shuf = y[idx]
                
                y_pred_all = np.dot(X, W) + b
                error = y_pred_all - y
                
                dW = (2/len(X)) * np.dot(X.T, error)
                db = (2/len(X)) * np.sum(error)
                
                W -= lr * dW
                b -= lr * db
                
                y_pred_all = np.dot(X, W) + b
                mse = float(np.mean((y_pred_all - y)**2))
                mae = float(np.mean(np.abs(y_pred_all - y)))
                r2 = get_r2(y, y_pred_all)
                
                if epoch == 10:
                    yield format_sse("status", {"message": "Epoch 10: Adjusting the knobs — error is starting to decrease!", "phase": "learning"})
                elif epoch == 50:
                    yield format_sse("status", {"message": "Epoch 50: The AI is finding the pattern! R² is climbing...", "phase": "learning"})
                elif epoch == 100:
                    yield format_sse("status", {"message": "Epoch 100: Fine-tuning — predictions are getting close to correct answers...", "phase": "fine-tuning"})
                
                top_25_pred = y_pred_all[:25].tolist()
                top_25_act = y[:25].tolist()
                
                yield format_sse("epoch", {
                    "epoch": epoch,
                    "total": epochs,
                    "loss": mse,
                    "r2": r2,
                    "mae": mae,
                    "weights": [float(b)] + [float(w) for w in W],
                    "predictions": top_25_pred,
                    "actuals": top_25_act
                })
                
                time.sleep(0.180)
                
            yield format_sse("status", {"message": "Training complete! The AI has learned the relationship between sensors and dosing.", "phase": "complete"})
            
            params_dict = {
                "model_type": "linear",
                "w_dosing": [float(b)] + [float(w) for w in W]
            }
            temp_path = os.path.join(os.path.dirname(__file__), 'model_params.json')
            with open(temp_path, 'w') as f:
                json.dump(params_dict, f, indent=2)
                
            yield format_sse("complete", {
                "r2": r2,
                "mae": mae,
                "model_type": "linear",
                "weights": params_dict["w_dosing"]
            })

        else:
            np.random.seed(42)
            W1 = np.random.randn(4, 8) * 0.1
            b1 = np.zeros(8)
            W2 = np.random.randn(8, 1) * 0.1
            b2 = np.zeros(1)
            
            lr = 0.01
            epochs = 200
            
            for epoch in range(1, epochs + 1):
                Z1 = np.dot(X, W1) + b1
                A1 = np.maximum(0, Z1)
                Z2 = np.dot(A1, W2) + b2
                y_pred = Z2.flatten()
                
                error = y_pred - y
                mse = float(np.mean(error**2))
                mae = float(np.mean(np.abs(error)))
                r2 = get_r2(y, y_pred)
                
                m = len(X)
                dZ2 = error.reshape(-1, 1)
                dW2 = (2/m) * np.dot(A1.T, dZ2)
                db2 = (2/m) * np.sum(dZ2, axis=0)
                
                dA1 = np.dot(dZ2, W2.T)
                dZ1 = dA1 * (Z1 > 0)
                dW1 = (2/m) * np.dot(X.T, dZ1)
                db1 = (2/m) * np.sum(dZ1, axis=0)
                
                W1 -= lr * dW1
                b1 -= lr * db1
                W2 -= lr * dW2
                b2 -= lr * db2
                
                if epoch == 10:
                    yield format_sse("status", {"message": "Epoch 10: Adjusting the knobs — error is starting to decrease!", "phase": "learning"})
                elif epoch == 50:
                    yield format_sse("status", {"message": "Epoch 50: The AI is finding the pattern! R² is climbing...", "phase": "learning"})
                elif epoch == 100:
                    yield format_sse("status", {"message": "Epoch 100: Fine-tuning — predictions are getting close to correct answers...", "phase": "fine-tuning"})
                    
                top_25_pred = y_pred[:25].tolist()
                top_25_act = y[:25].tolist()
                
                yield format_sse("epoch", {
                    "epoch": epoch,
                    "total": epochs,
                    "loss": mse,
                    "r2": r2,
                    "mae": mae,
                    "weights": [],
                    "w1": W1.T.tolist(),
                    "w2": W2.flatten().tolist(),
                    "b1": b1.tolist(),
                    "b2": float(b2[0]),
                    "predictions": top_25_pred,
                    "actuals": top_25_act
                })
                
                time.sleep(0.130)
                
            yield format_sse("status", {"message": "Training complete! The AI has learned the relationship between sensors and dosing.", "phase": "complete"})
            
            params_dict = {
                "model_type": "mlp",
                "w1": W1.T.tolist(),
                "b1": b1.tolist(),
                "w2": W2.flatten().tolist(),
                "b2": float(b2[0])
            }
            temp_path = os.path.join(os.path.dirname(__file__), 'model_params.json')
            with open(temp_path, 'w') as f:
                json.dump(params_dict, f, indent=2)
                
            yield format_sse("complete", {
                "r2": r2,
                "mae": mae,
                "model_type": "mlp"
            })

    return Response(generate(), mimetype='text/event-stream')

@app.route('/download')
def download():
    path = os.path.join(os.path.dirname(__file__), 'model_params.json')
    if not os.path.exists(path):
        return "Model not trained yet", 400
    return send_file(path, as_attachment=True, download_name='model_params.json')

if __name__ == '__main__':
    if os.environ.get('WERKZEUG_RUN_MAIN') == 'true' or not app.debug:
        sync_thread = threading.Thread(target=background_sync_loop, daemon=True)
        sync_thread.start()
        
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port, debug=False)
