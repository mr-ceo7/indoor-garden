import os
import json
import time
import requests
import threading
from io import StringIO
import numpy as np
import pandas as pd
from flask import Flask, request, jsonify, render_template_string, send_file
from sklearn.linear_model import LinearRegression
from sklearn.neural_network import MLPRegressor

app = Flask(__name__)

# Cache states for auto-training
last_seen_rows = 0
last_r2_score = -1.0

# ═══════════════════════════════════════════════════════════
#  PREMIUM GLASSMORPHIC HTML TEMPLATE
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
    .btn-danger {
      background: linear-gradient(135deg, #334155 0%, #1e293b 100%);
      box-shadow: 0 0 12px rgba(51, 65, 85, 0.2);
    }
    .btn-danger:hover {
      box-shadow: 0 0 20px rgba(51, 65, 85, 0.4);
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
    .metrics {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 16px;
      margin-bottom: 24px;
    }
    .metric-card {
      background: rgba(10, 20, 15, 0.6);
      border: 1px solid var(--border-color);
      padding: 16px;
      border-radius: 8px;
      text-align: center;
    }
    .metric-val {
      font-size: 1.6rem;
      font-weight: bold;
      color: var(--glow-color);
      margin-bottom: 4px;
    }
    .metric-label {
      font-size: 0.85rem;
      color: var(--text-muted);
    }
    .table-wrapper {
      max-height: 250px;
      overflow-y: auto;
      border: 1px solid var(--border-color);
      border-radius: 8px;
      margin-top: 16px;
    }
    table {
      width: 100%;
      border-collapse: collapse;
    }
    th, td {
      border-bottom: 1px solid var(--border-color);
      padding: 10px;
      text-align: left;
      font-size: 0.9rem;
    }
    th {
      color: var(--glow-color);
      background-color: #050d0a;
      position: sticky;
      top: 0;
    }
    pre {
      background-color: #020604;
      border: 1px solid var(--border-color);
      padding: 16px;
      border-radius: 8px;
      overflow-x: auto;
      color: #10b981;
      max-height: 250px;
      font-size: 0.85rem;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Garden-AI Studio</h1>
      <p class="subtitle">Edge AI Nutrient Optimization Calibration Engine</p>
    </header>

    {% if not trained %}
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
    {% else %}
    <div class="panel">
      <h2>Model Evaluation & Validation Metrics</h2>
      
      <div class="metrics">
        <div class="metric-card">
          <div class="metric-val">{{ r2_score }}</div>
          <div class="metric-label">R&sup2; Score (Coeff. of Determination)</div>
        </div>
        <div class="metric-card">
          <div class="metric-val">{{ mae_score }}s</div>
          <div class="metric-label">Mean Absolute Error</div>
        </div>
      </div>

      <h2>Actual vs. Predicted Dosing Times</h2>
      <div class="table-wrapper">
        <table>
          <thead>
            <tr>
              <th>Sample #</th>
              <th>Actual Duration</th>
              <th>Predicted Duration</th>
              <th>Absolute Error</th>
            </tr>
          </thead>
          <tbody>
            {% for r in results_table %}
            <tr>
              <td>{{ r.idx }}</td>
              <td>{{ r.actual }}s</td>
              <td>{{ r.pred }}s</td>
              <td>{{ r.err }}s</td>
            </tr>
            {% endfor %}
          </tbody>
        </table>
      </div>
    </div>

    <div class="panel">
      <h2>Download Edge Parameters</h2>
      <p style="margin-bottom: 20px; color: var(--text-muted); font-size: 0.95rem;">
        Save the parameter file below and drag-and-drop it directly into your Garden-AI Control Center web portal.
      </p>
      <a href="/download" class="btn" style="margin-bottom: 20px;">Download model_params.json</a>
      <a href="/" class="btn btn-danger">Train Another Model</a>

      <h2 style="margin-top: 30px;">Model Parameters Preview</h2>
      <pre><code>{{ params_preview }}</code></pre>
    </div>
    {% endif %}

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
    return render_template_string(HTML_TEMPLATE, trained=False)

@app.route('/train', methods=['POST'])
def train():
    global last_r2_score
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
    
    params_dict = {}
    
    if model_type == 'linear':
        model = LinearRegression()
        model.fit(X, y)
        y_pred = model.predict(X)
        
        params_dict = {
            "model_type": "linear",
            "w_dosing": [
                float(model.intercept_),
                float(model.coef_[0]),
                float(model.coef_[1]),
                float(model.coef_[2]),
                float(model.coef_[3])
            ]
        }
    else:
        model = MLPRegressor(
            hidden_layer_sizes=(8,),
            activation='relu',
            solver='lbfgs',
            max_iter=5000,
            random_state=42
        )
        model.fit(X, y)
        y_pred = model.predict(X)
        
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

    r2 = float(np.clip(1.0 - (np.sum((y - y_pred)**2) / np.sum((y - np.mean(y))**2)), 0.0, 1.0))
    last_r2_score = r2
    
    mae = float(np.mean(np.abs(y - y_pred)))
    
    temp_path = os.path.join(os.path.dirname(__file__), 'model_params.json')
    with open(temp_path, 'w') as f:
        json.dump(params_dict, f, indent=2)
        
    results_table = []
    for idx, (act, prd) in enumerate(zip(y[:40], y_pred[:40])):
        err = abs(act - prd)
        results_table.append({
            "idx": idx + 1,
            "actual": f"{act:.1f}",
            "pred": f"{prd:.1f}",
            "err": f"{err:.1f}"
        })
        
    preview = json.dumps(params_dict, indent=2)
    
    return render_template_string(
        HTML_TEMPLATE,
        trained=True,
        r2_score=f"{r2:.3f}",
        mae_score=f"{mae:.2f}",
        results_table=results_table,
        params_preview=preview
    )

@app.route('/download')
def download():
    path = os.path.join(os.path.dirname(__file__), 'model_params.json')
    if not os.path.exists(path):
        return "Model not trained yet", 400
    return send_file(path, as_attachment=True, download_name='model_params.json')

if __name__ == '__main__':
    # Ensure background thread spawns only once
    if os.environ.get('WERKZEUG_RUN_MAIN') == 'true' or not app.debug:
        sync_thread = threading.Thread(target=background_sync_loop, daemon=True)
        sync_thread.start()
        
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port, debug=False)

