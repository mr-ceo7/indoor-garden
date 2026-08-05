#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char HTTP_INDEX[] PROGMEM = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Garden AI Portal</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=Outfit:wght@400;500;600;700&display=swap');
    
    :root {
      --bg-dark: #04080e;
      --bg-sidebar: rgba(6, 12, 24, 0.85);
      --bg-card: rgba(10, 20, 30, 0.6);
      --border-glow: rgba(16, 185, 129, 0.15);
      --primary: #10b981;
      --primary-glow: rgba(16, 185, 129, 0.25);
      --accent: #a855f7;
      --accent-glow: rgba(168, 85, 247, 0.2);
      --info: #0ea5e9;
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
        radial-gradient(circle at 20% 20%, rgba(16, 185, 129, 0.05) 0%, transparent 40%),
        radial-gradient(circle at 80% 80%, rgba(168, 85, 247, 0.04) 0%, transparent 40%);
      color: var(--text-main);
      height: 100vh;
      width: 100vw;
      display: flex;
      overflow: hidden;
    }

    /* ─── Sidebar Navigation ────────────────────────────────── */
    .sidebar {
      width: 240px;
      background: var(--bg-sidebar);
      border-right: 1px solid rgba(255, 255, 255, 0.04);
      display: flex;
      flex-direction: column;
      padding: 24px 16px;
      flex-shrink: 0;
      z-index: 10;
      backdrop-filter: blur(20px);
    }

    .sidebar-brand {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-bottom: 36px;
      padding-left: 8px;
    }

    .brand-logo {
      width: 28px;
      height: 28px;
      fill: var(--primary);
    }

    .brand-logo-text {
      font-family: 'Outfit', sans-serif;
      font-size: 1.25rem;
      font-weight: 700;
      background: linear-gradient(135deg, var(--primary) 0%, #34d399 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    .nav-menu {
      display: flex;
      flex-direction: column;
      gap: 6px;
      flex: 1;
    }

    .nav-item {
      display: flex;
      align-items: center;
      gap: 14px;
      padding: 12px 16px;
      border-radius: 10px;
      color: var(--text-muted);
      cursor: pointer;
      font-weight: 500;
      font-size: 0.92rem;
      transition: all 0.2s ease;
      position: relative;
    }

    .nav-item svg {
      width: 18px;
      height: 18px;
      fill: currentColor;
    }

    .nav-item:hover {
      color: var(--text-main);
      background: rgba(255, 255, 255, 0.02);
    }

    .nav-item.active {
      color: var(--text-main);
      background: rgba(16, 185, 129, 0.08);
      font-weight: 600;
    }

    .nav-item.active::before {
      content: '';
      position: absolute;
      left: 0;
      top: 25%;
      height: 50%;
      width: 4px;
      background: var(--primary);
      border-radius: 0 4px 4px 0;
      box-shadow: 0 0 10px var(--primary);
    }

    /* ─── Main Content Container ────────────────────────────── */
    .main-content {
      flex: 1;
      display: flex;
      flex-direction: column;
      padding: 20px 24px;
      min-width: 0;
      height: 100%;
      overflow: hidden;
    }

    /* ─── Header Bar ────────────────────────────────────────── */
    .header-bar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
      flex-shrink: 0;
    }

    .welcome-text h1 {
      font-family: 'Outfit', sans-serif;
      font-size: 1.8rem;
      font-weight: 600;
      color: var(--text-main);
    }

    .header-right {
      display: flex;
      align-items: center;
      gap: 20px;
    }

    .notification-bell {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid rgba(255, 255, 255, 0.04);
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      color: var(--text-muted);
      position: relative;
      transition: all 0.2s;
    }

    .notification-bell:hover {
      background: rgba(255, 255, 255, 0.05);
      color: var(--text-main);
    }

    .notification-dot {
      position: absolute;
      top: 10px;
      right: 11px;
      width: 6px;
      height: 6px;
      border-radius: 50%;
      background: var(--accent);
      box-shadow: 0 0 6px var(--accent);
    }

    .user-profile {
      display: flex;
      align-items: center;
      gap: 12px;
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid rgba(255, 255, 255, 0.04);
      padding: 6px 14px 6px 6px;
      border-radius: 99px;
    }

    .user-avatar {
      width: 32px;
      height: 32px;
      border-radius: 50%;
      background: #1e293b;
      object-fit: cover;
    }

    .user-name {
      font-size: 0.88rem;
      font-weight: 600;
      color: var(--text-main);
    }

    /* ─── Dashboard Content Layout ──────────────────────────── */
    .dashboard-layout {
      flex: 1;
      display: flex;
      gap: 20px;
      min-height: 0;
      overflow: hidden;
    }

    /* Left Card: System Visualizer */
    .visualizer-card {
      width: 260px;
      background: var(--bg-card);
      border: 1px solid rgba(255, 255, 255, 0.04);
      border-radius: 16px;
      padding: 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: space-between;
      backdrop-filter: blur(16px);
      flex-shrink: 0;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
    }

    .visualizer-status {
      font-size: 0.72rem;
      font-weight: 700;
      color: var(--primary);
      text-transform: uppercase;
      letter-spacing: 1.5px;
      display: flex;
      align-items: center;
      gap: 6px;
      animation: blinker 2.5s infinite;
    }

    .visualizer-status::before {
      content: '';
      width: 6px;
      height: 6px;
      border-radius: 50%;
      background: var(--primary);
      box-shadow: 0 0 8px var(--primary);
    }

    .visualizer-status.anomaly {
      color: var(--danger);
    }

    .visualizer-status.anomaly::before {
      background: var(--danger);
      box-shadow: 0 0 8px var(--danger);
    }

    .stage {
      flex: 1;
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 10px 0;
      min-height: 0;
    }

    .stage svg {
      max-height: 100%;
      max-width: 100%;
      filter: drop-shadow(0 0 10px rgba(16, 185, 129, 0.1));
    }

    /* Right Area: Grid & Controls */
    .dashboard-main {
      flex: 1;
      display: flex;
      flex-direction: column;
      gap: 20px;
      min-width: 0;
    }

    /* Gauge Grid */
    .gauge-row {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 16px;
      flex-shrink: 0;
    }

    .gauge-card {
      background: var(--bg-card);
      border: 1px solid rgba(255, 255, 255, 0.04);
      border-radius: 16px;
      padding: 14px 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
      position: relative;
      backdrop-filter: blur(16px);
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
    }

    .gauge-card-header {
      width: 100%;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 6px;
    }

    .gauge-title {
      font-size: 0.8rem;
      font-weight: 500;
      color: var(--text-muted);
    }

    .gauge-pct {
      font-size: 0.72rem;
      font-weight: 700;
      color: var(--primary);
      background: rgba(16, 185, 129, 0.08);
      padding: 2px 6px;
      border-radius: 99px;
    }

    .gauge-ring-wrapper {
      position: relative;
      width: 84px;
      height: 84px;
      display: flex;
      align-items: center;
      justify-content: center;
      margin-bottom: 6px;
    }

    .gauge-ring-svg {
      width: 100%;
      height: 100%;
      transform: rotate(-90deg);
    }

    .ring-bg {
      fill: none;
      stroke: rgba(255, 255, 255, 0.03);
      stroke-width: 3.2;
    }

    .ring-fill {
      fill: none;
      stroke-width: 3.2;
      stroke-linecap: round;
      transition: stroke-dasharray 0.5s ease;
    }

    .ring-fill.temp { stroke: #f97316; }
    .ring-fill.hum { stroke: #0ea5e9; }
    .ring-fill.ph { stroke: #a855f7; }
    .ring-fill.ec { stroke: #10b981; }

    .gauge-value {
      position: absolute;
      font-size: 1.15rem;
      font-weight: 700;
      font-family: 'Outfit', sans-serif;
      text-shadow: 0 0 10px rgba(0,0,0,0.3);
    }

    .gauge-limits {
      font-size: 0.65rem;
      color: var(--text-muted);
      font-family: monospace;
    }

    /* Control Panel Card */
    .control-card {
      flex: 1;
      background: var(--bg-card);
      border: 1px solid rgba(255, 255, 255, 0.04);
      border-radius: 16px;
      padding: 20px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      backdrop-filter: blur(16px);
      min-height: 0;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
    }

    .control-header {
      font-size: 0.95rem;
      font-weight: 600;
      color: var(--text-main);
      margin-bottom: 12px;
      flex-shrink: 0;
    }

    .control-panel-grid {
      display: grid;
      grid-template-columns: 1.1fr 1fr;
      gap: 24px;
      flex: 1;
      min-height: 0;
    }

    .control-left, .control-right {
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      height: 100%;
    }

    .control-left { border-right: 1px solid rgba(255, 255, 255, 0.04); padding-right: 24px; }

    .control-section-lbl {
      font-size: 0.72rem;
      font-weight: 600;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.5px;
      margin-bottom: 8px;
    }

    .btn-group {
      display: flex;
      gap: 8px;
      margin-bottom: 14px;
    }

    .btn-opt {
      flex: 1;
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid rgba(255, 255, 255, 0.06);
      padding: 10px;
      border-radius: 8px;
      color: var(--text-muted);
      cursor: pointer;
      font-weight: 600;
      font-size: 0.82rem;
      transition: all 0.2s ease;
      text-align: center;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 2px;
    }

    .btn-opt-sub {
      font-size: 0.6rem;
      font-weight: 400;
      color: transparent;
      transition: color 0.2s;
    }

    .btn-opt.active {
      background: rgba(16, 185, 129, 0.08);
      color: var(--primary);
      border-color: rgba(16, 185, 129, 0.3);
      box-shadow: inset 0 0 10px rgba(16, 185, 129, 0.1);
    }

    .btn-opt.active .btn-opt-sub {
      color: var(--text-muted);
    }

    .btn-opt#opt-ai.active {
      background: rgba(168, 85, 247, 0.08);
      color: var(--accent);
      border-color: rgba(168, 85, 247, 0.3);
      box-shadow: inset 0 0 10px rgba(168, 85, 247, 0.1);
    }

    .btn-opt#opt-ai.active .btn-opt-sub {
      color: var(--accent);
      animation: blinker 2s infinite;
    }

    .slider-section {
      background: rgba(0,0,0,0.15);
      border-radius: 10px;
      padding: 12px;
      border: 1px solid rgba(255, 255, 255, 0.02);
    }

    .slider-header {
      display: flex;
      justify-content: space-between;
      margin-bottom: 8px;
      font-size: 0.78rem;
    }

    .slider-val {
      font-family: monospace;
      font-weight: 700;
      color: var(--primary);
    }

    .slider-input-wrapper {
      display: flex;
      align-items: center;
      gap: 10px;
    }

    input[type=range] {
      flex: 1;
      height: 4px;
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
      box-shadow: 0 0 8px var(--primary-glow);
      transition: transform 0.1s;
    }

    input[type=range]::-webkit-slider-thumb:hover {
      transform: scale(1.15);
    }

    .btn-pump {
      width: 100%;
      background: linear-gradient(135deg, var(--primary) 0%, #059669 100%);
      color: white;
      border: none;
      padding: 12px;
      border-radius: 10px;
      font-size: 0.88rem;
      font-weight: 700;
      cursor: pointer;
      box-shadow: 0 4px 12px var(--primary-glow);
      transition: all 0.2s ease;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }

    .btn-pump:hover {
      transform: translateY(-1px);
      box-shadow: 0 6px 16px var(--primary-glow);
    }

    .btn-pump:disabled {
      background: #1e293b !important;
      color: var(--text-muted) !important;
      cursor: not-allowed;
      box-shadow: none !important;
      transform: none !important;
    }

    .action-row {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: auto;
    }

    .btn-record {
      flex: 1;
      background: linear-gradient(135deg, var(--accent) 0%, #7c3aed 100%);
      color: white;
      border: none;
      padding: 12px;
      border-radius: 10px;
      font-size: 0.88rem;
      font-weight: 700;
      cursor: pointer;
      box-shadow: 0 4px 14px var(--accent-glow);
      transition: all 0.2s ease;
    }

    .btn-record:hover {
      transform: translateY(-1px);
      box-shadow: 0 6px 18px var(--accent-glow);
    }

    .btn-settings {
      width: 44px;
      height: 44px;
      border-radius: 10px;
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid rgba(255, 255, 255, 0.05);
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      color: var(--text-muted);
      transition: all 0.2s;
    }

    .btn-settings:hover {
      background: rgba(255, 255, 255, 0.06);
      color: var(--text-main);
      transform: rotate(45deg);
    }

    .btn-settings svg { width: 20px; height: 20px; fill: currentColor; }

    /* Active parameters summary inside control box */
    .model-info-widget {
      background: rgba(255,255,255,0.01);
      border: 1px solid rgba(255,255,255,0.03);
      border-radius: 10px;
      padding: 12px;
      font-size: 0.78rem;
    }

    .model-info-row {
      display: flex;
      justify-content: space-between;
      padding: 6px 0;
      border-bottom: 1px solid rgba(255,255,255,0.02);
    }

    .model-info-row:last-child { border-bottom: none; }
    .model-info-row span:last-child { font-weight: 700; font-family: monospace; }



    .chat-drawer {
      position: fixed;
      top: 0;
      right: -360px;
      width: 340px;
      height: 100vh;
      background: rgba(5, 8, 16, 0.93);
      border-left: 1px solid rgba(255, 255, 255, 0.05);
      backdrop-filter: blur(24px);
      box-shadow: -10px 0 30px rgba(0, 0, 0, 0.5);
      z-index: 100;
      transition: right 0.3s cubic-bezier(0.075, 0.82, 0.165, 1);
      display: flex;
      flex-direction: column;
    }

    .chat-drawer.open { right: 0; }

    .chat-drawer-header {
      padding: 18px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.04);
      display: flex;
      justify-content: space-between;
      align-items: center;
      background: rgba(255,255,255,0.01);
    }

    .chat-drawer-title {
      font-family: 'Outfit', sans-serif;
      font-size: 1rem;
      font-weight: 600;
      color: var(--accent);
      display: flex;
      align-items: center;
      gap: 8px;
    }

    .chat-drawer-close {
      background: transparent;
      border: none;
      color: var(--text-muted);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: color 0.2s;
    }

    .chat-drawer-close:hover { color: var(--text-main); }
    .chat-drawer-close svg { width: 20px; height: 20px; fill: currentColor; }

    .chat-container {
      flex: 1;
      display: flex;
      flex-direction: column;
      min-height: 0;
    }

    .chat-messages {
      flex: 1;
      padding: 16px;
      overflow-y: auto;
      display: flex;
      flex-direction: column;
      gap: 12px;
    }

    .chat-bubble {
      max-width: 85%;
      padding: 10px 14px;
      border-radius: 12px;
      font-size: 0.82rem;
      line-height: 1.4;
      word-wrap: break-word;
    }

    .chat-bubble.user {
      align-self: flex-end;
      background: rgba(16, 185, 129, 0.15);
      border: 1px solid rgba(16, 185, 129, 0.25);
      color: #f8fafc;
      border-bottom-right-radius: 2px;
    }

    .chat-bubble.assistant {
      align-self: flex-start;
      background: rgba(168, 85, 247, 0.1);
      border: 1px solid rgba(168, 85, 247, 0.2);
      color: #f8fafc;
      border-bottom-left-radius: 2px;
    }

    .chat-presets {
      display: flex;
      gap: 6px;
      padding: 8px 16px;
      background: rgba(0,0,0,0.2);
      border-top: 1px solid rgba(255, 255, 255, 0.03);
      overflow-x: auto;
      scrollbar-width: none;
      flex-shrink: 0;
    }

    .chat-presets::-webkit-scrollbar { display: none; }

    .preset-btn {
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid rgba(255, 255, 255, 0.05);
      border-radius: 99px;
      color: var(--text-muted);
      padding: 4px 12px;
      font-size: 0.72rem;
      cursor: pointer;
      white-space: nowrap;
      transition: all 0.2s ease;
    }

    .preset-btn:hover {
      background: rgba(168, 85, 247, 0.12);
      border-color: rgba(168, 85, 247, 0.25);
      color: var(--text-main);
    }

    .chat-input-bar {
      display: flex;
      align-items: center;
      padding: 10px 16px;
      background: rgba(5, 8, 16, 0.95);
      border-top: 1px solid rgba(255, 255, 255, 0.04);
      gap: 8px;
      flex-shrink: 0;
    }

    .chat-input {
      flex: 1;
      background: rgba(255,255,255,0.02);
      border: 1px solid rgba(255,255,255,0.05);
      border-radius: 20px;
      color: white;
      padding: 8px 16px;
      outline: none;
      font-size: 0.8rem;
    }

    .chat-send-btn {
      background: var(--accent);
      border: none;
      color: white;
      width: 32px;
      height: 32px;
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

    .chat-send-btn svg { width: 14px; height: 14px; fill: currentColor; }

    /* ─── Settings Modal Overlay ────────────────────────────── */
    .modal-overlay {
      position: fixed;
      top: 0;
      left: 0;
      width: 100vw;
      height: 100vh;
      background: rgba(4, 6, 10, 0.85);
      backdrop-filter: blur(12px);
      z-index: 200;
      display: none;
      align-items: center;
      justify-content: center;
      opacity: 0;
      transition: opacity 0.2s ease;
    }

    .modal-overlay.open { display: flex; opacity: 1; }

    .modal {
      background: rgba(10, 20, 30, 0.95);
      border: 1px solid rgba(255,255,255,0.06);
      width: 90%;
      max-width: 480px;
      border-radius: 16px;
      padding: 24px;
      box-shadow: 0 15px 40px rgba(0, 0, 0, 0.6);
      display: flex;
      flex-direction: column;
      gap: 16px;
      transform: scale(0.95);
      transition: transform 0.2s ease;
    }

    .modal-overlay.open .modal { transform: scale(1); }

    .modal-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid rgba(255,255,255,0.04);
      padding-bottom: 12px;
    }

    .modal-title { font-family: 'Outfit', sans-serif; font-size: 1.1rem; font-weight: 700; color: var(--primary); }

    .modal-close { background: transparent; border: none; color: var(--text-muted); cursor: pointer; }
    .modal-close:hover { color: var(--text-main); }
    .modal-close svg { width: 22px; height: 22px; fill: currentColor; }

    .modal-section { display: flex; flex-direction: column; gap: 8px; }
    .modal-btn {
      width: 100%;
      padding: 10px;
      border-radius: 8px;
      border: none;
      font-weight: 700;
      font-size: 0.82rem;
      cursor: pointer;
      transition: all 0.2s;
    }

    .modal-btn.danger { background: rgba(239, 68, 68, 0.1); border: 1px solid rgba(239,68,68,0.25); color: var(--danger); }
    .modal-btn.danger:hover { background: rgba(239, 68, 68, 0.2); }

    .modal-btn.primary { background: linear-gradient(135deg, var(--primary) 0%, #059669 100%); color: white; }
    .modal-btn.primary:hover { box-shadow: 0 0 12px var(--primary-glow); }

    .modal-btn.secondary { background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.06); color: white; }
    .modal-btn.secondary:hover { background: rgba(255,255,255,0.06); }

    .dropzone {
      border: 1.5px dashed rgba(168, 85, 247, 0.25);
      border-radius: 10px;
      padding: 24px;
      text-align: center;
      cursor: pointer;
      background: rgba(168, 85, 247, 0.01);
      transition: all 0.2s;
    }

    .dropzone:hover { border-color: var(--accent); background: rgba(168, 85, 247, 0.04); }
    .dropzone p { font-size: 0.78rem; color: var(--text-muted); margin: 0; }

    /* Thinking bounce animation */
    .thinking { display: flex; align-items: center; gap: 4px; padding: 10px 14px; }
    .thinking .dot {
      width: 5px; height: 5px; background: var(--accent); border-radius: 50%;
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
      50% { opacity: 0.5; }
    }
    
    /* ─── Responsive Media Queries ───────────────────────────── */
    @media (max-width: 900px) {
      body {
        flex-direction: column;
        height: auto;
        overflow-y: auto;
        overflow-x: hidden;
      }
      
      .sidebar {
        width: 100%;
        height: auto;
        flex-direction: row;
        justify-content: space-between;
        align-items: center;
        padding: 12px 20px;
        position: sticky;
        top: 0;
        border-right: none;
        border-bottom: 1px solid rgba(255, 255, 255, 0.04);
        box-shadow: 0 4px 20px rgba(0, 0, 0, 0.25);
      }
      
      .sidebar-brand {
        margin-bottom: 0;
        padding-left: 0;
      }
      
      .nav-menu {
        flex-direction: row;
        gap: 12px;
        flex: none;
      }
      
      .nav-item {
        padding: 8px 12px;
        font-size: 0.85rem;
      }
      
      .nav-item.active::before {
        left: 25%;
        top: auto;
        bottom: 0;
        width: 50%;
        height: 3px;
        border-radius: 4px 4px 0 0;
      }
      
      .main-content {
        padding: 16px;
        height: auto;
        overflow: visible;
      }
      
      .header-bar {
        flex-direction: column;
        align-items: flex-start;
        gap: 12px;
        margin-bottom: 16px;
      }
      

      
      .welcome-text h1 {
        font-size: 1.5rem;
      }
      
      .dashboard-layout {
        flex-direction: column;
        height: auto;
        overflow: visible;
        gap: 16px;
      }
      
      .visualizer-card {
        width: 100%;
        height: 320px;
        flex-direction: column;
        justify-content: space-between;
      }
      
      .gauge-row {
        grid-template-columns: repeat(2, 1fr);
        gap: 12px;
      }
      
      .control-panel-grid {
        grid-template-columns: 1fr;
        gap: 16px;
      }
      
      .control-left {
        border-right: none;
        border-bottom: 1px solid rgba(255, 255, 255, 0.04);
        padding-right: 0;
        padding-bottom: 16px;
      }
      

      
      .chat-drawer {
        z-index: 250;
      }
    }
    
    @media (max-width: 480px) {
      .gauge-row {
        grid-template-columns: 1fr;
      }
      
      .sidebar {
        padding: 10px 14px;
      }
      
      .sidebar-brand span {
        display: none;
      }
    }
  </style>
</head>
<body>

  <!-- ─── Sidebar Navigation ─── -->
  <div class="sidebar">
    <div class="sidebar-brand">
      <svg class="brand-logo" viewBox="0 0 24 24">
        <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 17h-2v-2h2v2zm2.07-7.75l-.9.92C13.45 12.9 13 13.5 13 15h-2v-.5c0-1.1.45-2.1 1.17-2.83l1.24-1.26c.37-.36.59-.86.59-1.41 0-1.1-.9-2-2-2s-2 .9-2 2H7c0-2.76 2.24-5 5-5s5 2.24 5 5c0 1.04-.42 1.99-1.07 2.75z"/>
      </svg>
      <span class="brand-logo-text">Garden-AI</span>
    </div>
    <div class="nav-menu">
      <div class="nav-item active">
        <svg viewBox="0 0 24 24"><path d="M3 13h8V3H3v10zm0 8h8v-6H3v6zm10 0h8V11h-8v10zm0-18v6h8V3h-8z"/></svg>
        <span>Dashboard</span>
      </div>
      <div class="nav-item" onclick="window.location.href='http://127.0.0.1:5000/'">
        <svg viewBox="0 0 24 24"><path d="M4 6H2v14c0 1.1.9 2 2 2h14v-2H4V6zm16-4H8c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H8V4h12v12z"/></svg>
        <span>AI Studio</span>
      </div>
      <div class="nav-item" onclick="toggleModal(true)">
        <svg viewBox="0 0 24 24"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/></svg>
        <span>Settings</span>
      </div>
      <div class="nav-item" id="nav-chat-item" onclick="toggleChat(true)">
        <svg viewBox="0 0 24 24"><path d="M20 2H4c-1.1 0-1.99.9-1.99 2L2 22l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM6 9h12v2H6V9zm8 5H6v-2h8v2zm4-6H6V6h12v2z"/></svg>
        <span>AI Assistant</span>
      </div>
    </div>
  </div>

  <!-- ─── Main Panel ─── -->
  <div class="main-content">
    <div class="header-bar">
      <div class="welcome-text">
        <h1>Welcome back, Kisii School!</h1>
      </div>
      <div class="header-right">
        <!-- Self learning sync overlay widget -->
        <div id="control-training-widget" style="display:none; display: flex; align-items: center; gap: 8px; background: rgba(168, 85, 247, 0.08); border: 1px solid rgba(168, 85, 247, 0.25); padding: 4px 10px; border-radius: 99px; font-size: 0.72rem; animation: blinker 2.5s infinite;">
          <span style="color:var(--accent); font-weight:700;">SELF-LEARNING OPTIMIZER ACTIVE: </span>
          <span id="widget-training-status" style="color:var(--text-main); font-family: monospace;">Synchronizing...</span>
        </div>
      </div>
    </div>

    <div class="dashboard-layout">
      <!-- Left Card: Hydroponic SVG visualizer -->
      <div class="visualizer-card">
        <div class="visualizer-status" id="anomaly-status-pill">
          <span id="anomaly-pill-text">SYSTEM STATUS: ACTIVE</span>
        </div>
        
        <div class="stage">
          <img src="http://127.0.0.1:5000/static/plant_tower.png" 
               onerror="this.style.display='none'; document.getElementById('fallback-svg').style.display='block';"
               style="max-width: 100%; max-height: 100%; object-fit: contain; border-radius: 8px; filter: drop-shadow(0 0 12px rgba(16, 185, 129, 0.15));" 
               alt="Plant Tower">
          <div id="fallback-svg" style="display: none; width: 100%; height: 100%;">
            <svg viewBox="0 0 200 200" style="width: 100%; height: 100%;">
              <!-- Hydroponic Tower / Pipes -->
              <rect x="85" y="15" width="30" height="120" rx="4" fill="#0b1329" stroke="var(--primary)" stroke-width="1.8" />
              
              <!-- Plants (Leafy Green) -->
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
              <rect x="45" y="135" width="110" height="50" rx="8" fill="rgba(11, 19, 41, 0.9)" stroke="rgba(255,255,255,0.06)" stroke-width="1.2" />
              
              <!-- Liquid level inside tank -->
              <rect id="liquid-level" x="48" y="145" width="104" height="36" rx="4" fill="url(#liquid-gradient)" style="transition: fill 0.6s ease;" />
              
              <!-- Flow lines when pump active -->
              <path id="water-flow" class="water-flow" d="M 100 135 L 100 25" fill="none" stroke="#0ea5e9" stroke-width="3" stroke-dasharray="6" stroke-dashoffset="0" style="display:none;" />
              
              <defs>
                <linearGradient id="liquid-gradient" x1="0%" y1="0%" x2="0%" y2="100%">
                  <stop id="grad-stop1" offset="0%" stop-color="#10b981" stop-opacity="0.85" />
                  <stop id="grad-stop2" offset="100%" stop-color="#047857" stop-opacity="0.95" />
                </linearGradient>
              </defs>
            </svg>
          </div>
        </div>

        <div class="visualizer-status" id="anomaly-status-footer-pill" style="opacity: 0.6; font-size: 0.65rem;">
          SYSTEM STATUS: ACTIVE
        </div>
      </div>

      <!-- Right Dashboard Content -->
      <div class="dashboard-main">
        <!-- 4 circular gauge cards row -->
        <div class="gauge-row">
          <!-- Temp -->
          <div class="gauge-card">
            <div class="gauge-card-header">
              <span class="gauge-title">Temperature</span>
              <span class="gauge-pct" id="gauge-temp-pct">0%</span>
            </div>
            <div class="gauge-ring-wrapper">
              <svg class="gauge-ring-svg" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-temp" class="ring-fill temp" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-value" id="lbl-temp">0°C</div>
            </div>
            <div class="gauge-limits">15°C / 35°C</div>
          </div>

          <!-- Humidity -->
          <div class="gauge-card">
            <div class="gauge-card-header">
              <span class="gauge-title">Humidity</span>
              <span class="gauge-pct" id="gauge-hum-pct">0%</span>
            </div>
            <div class="gauge-ring-wrapper">
              <svg class="gauge-ring-svg" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-hum" class="ring-fill hum" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-value" id="lbl-hum">0%</div>
            </div>
            <div class="gauge-limits">0% / 100%</div>
          </div>

          <!-- pH Level -->
          <div class="gauge-card">
            <div class="gauge-card-header">
              <span class="gauge-title">pH Level</span>
              <span class="gauge-pct" style="color:var(--accent); background:rgba(168,85,247,0.08)">5.5 - 6.5</span>
            </div>
            <div class="gauge-ring-wrapper">
              <svg class="gauge-ring-svg" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-ph" class="ring-fill ph" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-value" id="lbl-ph">7.0</div>
            </div>
            <div class="gauge-limits">5.0 / 8.0</div>
          </div>

          <!-- EC / Nutrients -->
          <div class="gauge-card">
            <div class="gauge-card-header">
              <span class="gauge-title">EC / Nutrients</span>
              <span class="gauge-pct" id="lbl-tds" style="color:var(--info); background:rgba(14,165,233,0.08)">TDS: 0 ppm</span>
            </div>
            <div class="gauge-ring-wrapper">
              <svg class="gauge-ring-svg" viewBox="0 0 36 36">
                <path class="ring-bg" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
                <path id="ring-ec" class="ring-fill ec" stroke-dasharray="0, 100" d="M18 2.0845 a 15.9155 15.9155 0 0 1 0 31.831 a 15.9155 15.9155 0 0 1 0 -31.831" />
              </svg>
              <div class="gauge-value" id="lbl-ec">0</div>
            </div>
            <div class="gauge-limits">0 / 800 µS/cm</div>
          </div>
        </div>

        <!-- Control Panel Card -->
        <div class="control-card">
          <div class="control-header">Control Panel</div>
          <div class="control-panel-grid">
            <!-- Left inputs side -->
            <div class="control-left">
              <div>
                <div class="control-section-lbl">Mode</div>
                <div class="btn-group">
                  <div class="btn-opt" id="opt-reactive" onclick="setMode('reactive')">
                    <span>Reactive</span>
                    <span class="btn-opt-sub">Threshold Dosing</span>
                  </div>
                  <div class="btn-opt" id="opt-manual" onclick="setMode('manual')">
                    <span>Manual</span>
                    <span class="btn-opt-sub">Direct Control</span>
                  </div>
                  <div class="btn-opt" id="opt-ai" onclick="setMode('ai')">
                    <span>AI-Edge</span>
                    <span class="btn-opt-sub">Smart Optimization</span>
                  </div>
                </div>
              </div>

              <!-- Timed dosing duration slider -->
              <div class="slider-section">
                <div class="slider-header">
                  <span class="gauge-title" style="color:var(--text-main); font-weight:600">Dosing Duration</span>
                  <span class="slider-val"><span id="slide-val-duration">5.0</span>s</span>
                </div>
                <div class="slider-input-wrapper">
                  <span style="font-size:0.65rem; color:var(--text-muted)">0s</span>
                  <input type="range" id="slide-duration" min="0.5" max="30.0" step="0.5" value="5.0" oninput="updateDosingSlider()">
                  <span style="font-size:0.65rem; color:var(--text-muted)">120s</span>
                </div>
              </div>
            </div>

            <!-- Right trigger button & calibration side -->
            <div class="control-right">
              <div>
                <div class="control-section-lbl">Actuator Control</div>
                <button class="btn-pump" id="btn-manual-pump" onclick="togglePump()" disabled>Turn Pump ON</button>
              </div>

              <!-- Active parameters summary inside control box -->
              <div class="model-info-widget">
                <div class="model-info-row">
                  <span>AI Expected Duration</span>
                  <span id="lbl-expected-dosing" style="color:var(--info)">0.0 s</span>
                </div>
                <div class="model-info-row">
                  <span>Active AI Model Type</span>
                  <span id="lbl-active-model-type" style="color:var(--accent)">None</span>
                </div>
                <div class="model-info-row">
                  <span>Pump Relay Switch</span>
                  <span id="lbl-pump-state" style="color:var(--text-muted)">OFF</span>
                </div>
              </div>

              <div class="action-row">
                <button class="btn-record" onclick="logDataPoint()">Record Calibration</button>
                <div class="btn-settings" onclick="toggleModal(true)">
                  <svg viewBox="0 0 24 24"><path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z"/></svg>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>



  <div class="chat-drawer" id="chat-drawer">
    <div class="chat-drawer-header">
      <div class="chat-drawer-title">
        <svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M21 15v4c0 1.1-.9 2-2 2H5c-1.1 0-2-.9-2-2v-4h2v4h14v-4h2zM5 12h14v2H5v-2zm8-8h-2v5H5v2h14V9h-6V4z"/></svg>
        <span>Garden-AI Chat Assistant</span>
      </div>
      <button class="chat-drawer-close" onclick="toggleChat(false)">
        <svg viewBox="0 0 24 24"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/></svg>
      </button>
    </div>

    <!-- Active training overlay screen inside chatbot side -->
    <div id="chat-training-overlay" style="display:none; position:absolute; top:0; left:0; width:100%; height:100%; background:rgba(5,8,14,0.95); backdrop-filter:blur(10px); z-index:10; flex-direction:column; align-items:center; justify-content:center; padding:20px; text-align:center;">
      <div style="width:100px; height:100px; margin-bottom:16px;">
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
      <div style="font-family:'Outfit', sans-serif; font-size:1rem; font-weight:600; color:var(--accent); margin-bottom:6px;">Optimizing Edge AI...</div>
      <div style="font-size:0.75rem; color:var(--text-muted); font-family:monospace; margin-bottom:12px; height:15px;" id="training-status-text">Fitting MLP Neural Network weights</div>
      <div style="width:120px; height:4px; background:rgba(255,255,255,0.05); border-radius:10px; overflow:hidden; position:relative;">
        <div style="position:absolute; height:100%; background:linear-gradient(90deg, var(--accent) 0%, var(--primary) 100%); border-radius:10px; animation: progress-run 4.5s infinite ease-in-out; width: 30%;"></div>
      </div>
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
        <input type="text" class="chat-input" id="chat-input" placeholder="Ask about crop health, pH, anomalies..." onkeydown="if(event.key === 'Enter') sendChatMessage()">
        <button class="chat-send-btn" onclick="sendChatMessage()">
          <svg viewBox="0 0 24 24"><path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z"/></svg>
        </button>
      </div>
    </div>
  </div>

  <!-- ─── Settings Modal ─── -->
  <div class="modal-overlay" id="modal-overlay" onclick="toggleModal(false)">
    <div class="modal" onclick="event.stopPropagation()">
      <div class="modal-header">
        <div class="modal-title">Settings & Calibration Manager</div>
        <button class="modal-close" onclick="toggleModal(false)">
          <svg viewBox="0 0 24 24"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/></svg>
        </button>
      </div>
      
      <div class="modal-section">
        <div class="control-section-lbl">Calibration Dataset Log</div>
        <div style="display:flex; gap:10px;">
          <a href="/download_csv" class="modal-btn secondary" style="text-align:center; flex:1.2;">Download dataset.csv</a>
          <button class="modal-btn danger" style="flex:1;" onclick="clearLog()">Clear Log</button>
        </div>
      </div>

      <div class="modal-section">
        <div class="control-section-lbl">Upload Trained Parameters (JSON)</div>
        <div class="dropzone" id="dropzone" onclick="document.getElementById('file-upload').click()">
          <p>Drag & Drop model_params.json here or click to upload</p>
          <input type="file" id="file-upload" style="display:none;" onchange="uploadModelFile(this.files[0])">
        </div>
      </div>

      <div class="modal-section" style="margin-top:10px;">
        <div class="control-section-lbl">System Calibration Actions</div>
        <button class="modal-btn danger" style="width:100%;" onclick="deleteModel()">Delete Active AI Model</button>
      </div>
    </div>
  </div>

  <script>
    let currentActiveMode = 'REACTIVE';
    let pumpTransitionActive = false;
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
      if (!pumpTransitionActive) {
        document.getElementById('btn-manual-pump').disabled = !isManual;
      }
    }

    function updateDosingSlider() {
      const duration = document.getElementById('slide-duration').value;
      document.getElementById('slide-val-duration').textContent = duration;
    }

    function updatePumpButtonState(state) {
      const manualBtn = document.getElementById('btn-manual-pump');
      if (state === 1) {
        manualBtn.textContent = 'Turn Pump OFF';
        manualBtn.style.background = "linear-gradient(135deg, var(--danger) 0%, #b91c1c 100%)";
        manualBtn.style.boxShadow = "0 3px 8px rgba(239, 68, 68, 0.25)";
      } else {
        manualBtn.textContent = 'Turn Pump ON';
        manualBtn.style.background = "linear-gradient(135deg, var(--primary) 0%, #059669 100%)";
        manualBtn.style.boxShadow = "0 3px 8px var(--primary-glow)";
      }
    }

    function togglePump() {
      if (pumpTransitionActive) return;
      
      const btn = document.getElementById('btn-manual-pump');
      const action = (systemStatus.pumpState === 0) ? 'ON' : 'OFF';
      
      pumpTransitionActive = true;
      btn.disabled = true;
      btn.textContent = (action === 'ON') ? 'Turning ON...' : 'Turning OFF...';
      
      fetch('/move?posH=' + document.getElementById('slide-duration').value + '&action=' + action, { method: 'POST' })
        .then(res => {
          if (res.ok) {
            systemStatus.pumpState = (action === 'ON') ? 1 : 0;
            updatePumpButtonState(systemStatus.pumpState);
            
            setTimeout(() => {
              pumpTransitionActive = false;
              if (currentActiveMode === 'MANUAL') {
                btn.disabled = false;
              }
            }, 2000);
          } else {
            pumpTransitionActive = false;
            if (currentActiveMode === 'MANUAL') {
              btn.disabled = false;
            }
            updatePumpButtonState(systemStatus.pumpState);
          }
        })
        .catch(() => {
          pumpTransitionActive = false;
          if (currentActiveMode === 'MANUAL') {
            btn.disabled = false;
          }
          updatePumpButtonState(systemStatus.pumpState);
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
          .then(msg => {
            alert(msg);
            toggleModal(false);
          });
      }
    }

    function deleteModel() {
      if (confirm("Are you sure you want to delete the active AI model from ESP-01 memory?")) {
        fetch('/delete_model', { method: 'POST' })
          .then(res => res.text())
          .then(msg => {
            alert(msg);
            toggleModal(false);
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
          toggleModal(false);
          fetchTelemetry();
        })
        .catch(err => {
          alert("Upload failed: network or connection error");
        });
      };
      reader.readAsText(file);
    }

    function toggleChat(open) {
      const drawer = document.getElementById('chat-drawer');
      const chatItem = document.getElementById('nav-chat-item');
      if (open) {
        drawer.classList.add('open');
        if (chatItem) chatItem.classList.add('active');
      } else {
        drawer.classList.remove('open');
        if (chatItem) chatItem.classList.remove('active');
      }
    }

    function toggleModal(open) {
      const modal = document.getElementById('modal-overlay');
      if (open) modal.classList.add('open');
      else modal.classList.remove('open');
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

          // Update gauge header details
          // Temperature percentage (scale 15°C to 35°C)
          let tempPct = Math.max(0, Math.min(100, Math.round(((data.temp - 15) / 20) * 100)));
          document.getElementById('gauge-temp-pct').textContent = tempPct + '%';
          document.getElementById('gauge-hum-pct').textContent = data.hum + '%';
          document.getElementById('lbl-tds').textContent = 'TDS: ' + Math.round(data.nc * 0.5) + ' ppm';

          // Update Circular Gauges
          // Temp (scale 15 to 35)
          let tFill = Math.max(0, Math.min(100, ((data.temp - 15) / 20) * 100));
          document.getElementById('ring-temp').style.strokeDasharray = tFill + ', 100';
          // Hum (max 100)
          document.getElementById('ring-hum').style.strokeDasharray = data.hum + ', 100';
          // pH (scale 5.0 to 8.0)
          let phFill = Math.max(0, Math.min(100, ((data.ph - 5.0) / 3.0) * 100));
          document.getElementById('ring-ph').style.strokeDasharray = phFill + ', 100';
          // EC / NPK (max 800)
          let ecFill = Math.max(0, Math.min(100, (data.nc / 800.0) * 100));
          document.getElementById('ring-ec').style.strokeDasharray = ecFill + ', 100';

          // Update pump details
          const pumpStateLbl = document.getElementById('lbl-pump-state');
          const waterFlowPath = document.getElementById('water-flow');
          if (data.pumpState === 1) {
            pumpStateLbl.textContent = "ACTIVE (ON)";
            pumpStateLbl.style.color = "var(--primary)";
            if (waterFlowPath) waterFlowPath.classList.add('active');
          } else {
            pumpStateLbl.textContent = "OFF";
            pumpStateLbl.style.color = "var(--text-muted)";
            if (waterFlowPath) waterFlowPath.classList.remove('active');
          }

          // Active Mode synchronization
          const serverMode = data.mode.toUpperCase();
          if (serverMode !== currentActiveMode) {
            updateModeUI(serverMode);
          }

          // Manual button styling if pump was toggled externally
          if (!pumpTransitionActive) {
            updatePumpButtonState(data.pumpState);
          }

          // Liquid visualizer height and color updates based on NC
          const stop1 = document.getElementById('grad-stop1');
          const stop2 = document.getElementById('grad-stop2');
          
          if (stop1 && stop2) {
            if (data.nc >= 507 || data.nc < 50) {
              stop1.setAttribute('stop-color', '#ef4444');
              stop2.setAttribute('stop-color', '#b91c1c');
            } else if (data.nc < 200) {
              stop1.setAttribute('stop-color', '#38bdf8');
              stop2.setAttribute('stop-color', '#0284c7');
            } else {
              stop1.setAttribute('stop-color', '#10b981');
              stop2.setAttribute('stop-color', '#047857');
            }
          }

          document.getElementById('lbl-expected-dosing').textContent = data.expected_dosing.toFixed(1) + ' s';

          // AI Model Status Indicator & Badge
          const activeModelTypeLbl = document.getElementById('lbl-active-model-type');
          if (data.ai_loaded) {
            const mType = data.model_type.toUpperCase();
            activeModelTypeLbl.textContent = mType;
            activeModelTypeLbl.style.color = "var(--primary)";
          } else {
            activeModelTypeLbl.textContent = "None";
            activeModelTypeLbl.style.color = "var(--text-muted)";
          }

          // Background self-learning training visualization
          const trainingOverlay = document.getElementById('chat-training-overlay');
          const trainingStatusText = document.getElementById('training-status-text');
          const trainingWidget = document.getElementById('control-training-widget');
          const trainingWidgetStatus = document.getElementById('widget-training-status');

          if (data.is_training) {
            trainingOverlay.style.display = "flex";
            trainingStatusText.textContent = data.training_status;
            trainingWidget.style.display = "flex";
            trainingWidgetStatus.textContent = data.training_status;
          } else {
            trainingOverlay.style.display = "none";
            trainingWidget.style.display = "none";
          }

          // Anomaly Health Banner update
          const healthPill = document.getElementById('anomaly-pill-text');
          const healthFooterPill = document.getElementById('anomaly-status-footer-pill');
          const anomalyCard = document.querySelector('.visualizer-card');

          if (data.anomaly_detected) {
            healthPill.textContent = "ANOMALY DETECTED";
            healthPill.parentElement.className = "visualizer-status anomaly";
            healthFooterPill.textContent = "DIAGNOSTIC ALERT";
            healthFooterPill.className = "visualizer-status anomaly";
            anomalyCard.style.borderColor = "rgba(239, 68, 68, 0.35)";
          } else {
            healthPill.textContent = "SYSTEM STATUS: ACTIVE";
            healthPill.parentElement.className = "visualizer-status";
            healthFooterPill.textContent = "SYSTEM STATUS: ACTIVE";
            healthFooterPill.className = "visualizer-status";
            anomalyCard.style.borderColor = "rgba(255, 255, 255, 0.04)";
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

#endif
