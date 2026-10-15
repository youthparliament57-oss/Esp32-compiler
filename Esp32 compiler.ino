#include <WiFi.h>
#include <WebServer.h>

// WiFi Hotspot details (Isse phone connect karein)
const char* ssid = "Ballie_Robot";
const char* password = "12345678";

WebServer server(80);

// =========================================================================
// HARDWARE PINS (ESP32-C3 Super Mini to DRV8833)
// =========================================================================
const int DRIVE_IN1 = 3;  // Motor A: 100 RPM Forward (IN1 / AIN1)
const int DRIVE_IN2 = 4;  // Motor A: 100 RPM Backward (IN2 / AIN2)
const int STEER_IN3 = 5;  // Motor B: L-Shape BO Left Tilt (IN3 / BIN1)
const int STEER_IN4 = 6;  // Motor B: L-Shape BO Right Tilt (IN4 / BIN2)
const int EEP_ENABLE = 7; // DRV8833 SLEEP/ENABLE Pin (Agar bypass kiya ho toh 3.3V direct de sakte hain)

// =========================================================================
// CALIBRATION & SPEEDS
// =========================================================================
int driveSpeed = 220; // 100 RPM main motor standard speed (0-255)
int steerSpeed = 150; // L-Shape Motor torque limitation (Over-rotation aur motor load bachane ke liye)

// =========================================================================
// RESPONSIVE SAMSUNG BALLIE YELLOW-THEME UI (Embedded HTML)
// =========================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="hi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Ballie AI Test Console</title>
  <style>
    :root {
      --yellow: #FFD600;
      --bg: #0d0d11;
      --card: #16161f;
      --text: #f5f5f7;
      --border: #2c2c38;
    }
    * { box-sizing: border-box; user-select: none; -webkit-user-select: none; }
    body {
      margin: 0; padding: 12px;
      background: var(--bg); color: var(--text);
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      display: flex; flex-direction: column; align-items: center; justify-content: center;
      min-height: 100vh;
    }
    .app-shell {
      width: 100%; max-width: 400px;
      background: #111116; border: 2px solid var(--border);
      border-radius: 28px; padding: 18px;
      box-shadow: 0 16px 40px rgba(0,0,0,0.75);
    }
    .header {
      display: flex; justify-content: space-between; align-items: center;
      margin-bottom: 15px;
    }
    .title { font-size: 20px; font-weight: 900; color: var(--yellow); letter-spacing: 0.8px; }
    .status-badge {
      background: #1c2e21; border: 1px solid #4caf50;
      color: #81c784; padding: 4px 10px; border-radius: 20px; font-size: 11px; font-weight: bold;
    }
    
    /* Dual View Live Mechanical Simulation */
    .viewport {
      background: radial-gradient(circle, #1a1a26 0%, #0a0a0f 100%);
      border: 1px solid var(--border); border-radius: 20px;
      height: 140px; position: relative; margin-bottom: 14px;
      display: flex; justify-content: space-around; align-items: center; overflow: hidden;
    }
    .hud-label {
      position: absolute; top: 6px; left: 10px; font-size: 9px; color: #777; font-weight: bold; letter-spacing: 0.5px;
    }
    .sim-panel {
      display: flex; flex-direction: column; align-items: center; position: relative; width: 45%;
    }
    .sim-caption { font-size: 10px; color: #999; margin-top: 6px; font-weight: bold; }
    
    /* 3D Circular Physics Representation */
    .ball-container {
      width: 70px; height: 70px; border-radius: 50%;
      background: radial-gradient(circle at 30% 30%, #fff176, var(--yellow) 65%, #c7a500 100%);
      border: 3px solid #000; position: relative;
      display: flex; align-items: center; justify-content: center;
      transition: transform 0.15s cubic-bezier(0.18, 0.89, 0.32, 1.28);
      box-shadow: 0 6px 15px rgba(255,214,0,0.25);
    }
    .ball-shading {
      position: absolute; width: 14px; height: 64px;
      border-left: 2px dashed rgba(0,0,0,0.3); border-right: 2px dashed rgba(0,0,0,0.3);
    }
    .lens {
      width: 32px; height: 12px; background: #08080c; border-radius: 8px;
      position: absolute; transform: translateY(-8px); display: flex; align-items: center; justify-content: center; gap: 3px;
    }
    .led { width: 5px; height: 5px; border-radius: 50%; background: #00e5ff; box-shadow: 0 0 6px #00e5ff; }
    
    .pendulum-hub {
      width: 4px; height: 4px; background: #fff; border-radius: 50%; position: absolute; top: 32px;
    }
    .pendulum-lever {
      position: absolute; width: 4px; height: 26px; background: #aaa;
      top: 34px; transform-origin: top center; transition: transform 0.15s ease-out;
    }
    .pendulum-mass {
      position: absolute; width: 22px; height: 12px; background: #3f51b5;
      bottom: 0; transform: translateX(-9px); border-radius: 3px; border: 1px solid #000;
    }

    /* Joystick controller buttons */
    .controls-container {
      display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 12px;
    }
    .control-block {
      background: var(--card); padding: 12px 10px; border-radius: 18px; border: 1px solid var(--border);
    }
    .block-title { font-size: 11px; color: #888; margin-bottom: 6px; font-weight: bold; letter-spacing: 0.5px; }
    .btn {
      width: 100%; height: 64px; margin: 5px 0;
      background: #1c1c24; color: var(--yellow);
      border: 2px solid var(--yellow); border-radius: 14px;
      font-size: 16px; font-weight: 800; cursor: pointer;
      transition: background 0.1s, color 0.1s;
    }
    .btn:active, .btn.active {
      background: var(--yellow); color: #000; border-color: var(--yellow); transform: scale(0.97);
    }
    
    /* Config Panel */
    .config-card {
      background: var(--card); border: 1px solid var(--border); padding: 12px 16px; border-radius: 16px; margin-bottom: 14px;
    }
    .config-row { display: flex; justify-content: space-between; font-size: 13px; font-weight: bold; margin-bottom: 4px; }
    input[type=range] { width: 100%; accent-color: var(--yellow); margin-top: 5px; cursor: pointer; }
    
    .actions { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
    .voice-btn {
      background: #17242c; color: #00e5ff; border: 2px solid #00e5ff;
      height: 48px; border-radius: 14px; font-weight: bold; font-size: 14px; cursor: pointer;
    }
    .voice-btn.listening { background: #00e5ff; color: #000; font-weight: 900; }
    .stop-btn {
      background: #3e1111; color: #ff5252; border: 2px solid #ff5252;
      height: 48px; border-radius: 14px; font-weight: bold; font-size: 14px; cursor: pointer;
    }
    .stop-btn:active { background: #ff5252; color: #000; }
    
    .console {
      margin-top: 10px; font-family: monospace; font-size: 10px; color: #666; text-align: left;
      background: #08080b; padding: 6px 10px; border-radius: 8px; border: 1px solid #1c1c24;
      max-height: 50px; overflow-y: auto;
    }
  </style>
</head>
<body>

<div class="app-shell">
  <div class="header">
    <div class="title">🟡 BALLIE CONTROL</div>
    <div class="status-badge" id="netStat">CONNECTED</div>
  </div>

  <div class="viewport">
    <div class="hud-label">LIVE MECHANICAL SIMULATION</div>
    
    <!-- View A: Side View Pitch (Motor Spin vs Shell Movement) -->
    <div class="sim-panel">
      <div class="ball-container" id="sideBall">
        <div class="ball-shading"></div>
        <div class="lens">
          <div class="led"></div><div class="led"></div><div class="led" style="background:#f50057; box-shadow:0 0 6px #f50057;"></div>
        </div>
      </div>
      <div class="sim-caption">SIDE PITCH (DRIVE)</div>
    </div>
    
    <!-- View B: Rear View Roll (Pendulum Tilt vs Path Turn) -->
    <div class="sim-panel">
      <div class="ball-container" id="rearBall" style="background: radial-gradient(circle, #e2b400, #967100); transform-origin: bottom center;">
        <div class="ball-shading" style="width: 58px; border-left:1px dashed #444; border-right:1px dashed #444;"></div>
        <div class="pendulum-hub"></div>
        <div class="pendulum-lever" id="pendulLeg">
          <div class="pendulum-mass"></div>
        </div>
      </div>
      <div class="sim-caption">REAR ROLL (STEER)</div>
    </div>
  </div>

  <div class="controls-container">
    <div class="control-block">
      <div class="block-title">100 RPM DRIVE (FWD/REV)</div>
      <button class="btn" id="btnF">▲ AAGE</button>
      <button class="btn" id="btnB">▼ PEECHE</button>
    </div>
    <div class="control-block">
      <div class="block-title">L-MOTOR TILT (STEER)</div>
      <button class="btn" id="btnL">◄ LEFT</button>
      <button class="btn" id="btnR">RIGHT ►</button>
    </div>
  </div>

  <div class="config-card">
    <div class="config-row">
      <span>Motor PWM Speed (Force)</span>
      <span id="spdVal" style="color:var(--yellow)">220 / 255</span>
    </div>
    <input type="range" min="100" max="255" value="220" id="spdSlider">
  </div>

  <div class="actions">
    <button class="voice-btn" id="voiceBtn">🎙️ VOICE COMMAND</button>
    <button class="stop-btn" id="stopBtn">🛑 STOP (RUKEIN)</button>
  </div>

  <div class="console" id="log">SYSTEM: Status OK. Dual Touch enabled.</div>
</div>

<script>
  let logText = document.getElementById('log');
  let sideBall = document.getElementById('sideBall');
  let rearBall = document.getElementById('rearBall');
  let pendulLeg = document.getElementById('pendulLeg');
  let spdSlider = document.getElementById('spdSlider');
  let spdVal = document.getElementById('spdVal');

  let currentPitch = 0;
  let currentRoll = 0;

  function logger(text) {
    logText.innerHTML = "LOG: " + text + "<br>" + logText.innerHTML;
  }

  // Animation Update Function for the Web SIM
  function renderSim(action) {
    switch (action) {
      case '/F': currentPitch -= 22; break; 
      case '/B': currentPitch += 22; break; 
      case '/SD': currentPitch = 0; break; 
      case '/L': currentRoll = -18; break; 
      case '/R': currentRoll = 18; break; 
      case '/CS': currentRoll = 0; break; 
      case '/S': currentPitch = 0; currentRoll = 0; break;
    }
    
    // Apply transformations based on simulated physical variables
    sideBall.style.transform = `rotate(${currentPitch}deg)`;
    rearBall.style.transform = `rotate(${currentRoll * 0.4}deg)`;
    pendulLeg.style.transform = `rotate(${-currentRoll}deg)`;
  }

  function callEndpoint(endpoint) {
    renderSim(endpoint);
    fetch(endpoint, { mode: 'no-cors' })
      .then(() => logger("Command success: " + endpoint))
      .catch(() => logger("Sent: " + endpoint));
  }

  function setupHoldingEvent(elementId, pressUrl, releaseUrl) {
    const el = document.getElementById(elementId);
    
    const onStart = (e) => {
      e.preventDefault();
      el.classList.add('active');
      callEndpoint(pressUrl);
    };
    
    const onEnd = (e) => {
      e.preventDefault();
      el.classList.remove('active');
      callEndpoint(releaseUrl);
    };

    el.addEventListener('touchstart', onStart, { passive: false });
    el.addEventListener('touchend', onEnd, { passive: false });
    el.addEventListener('mousedown', onStart);
    el.addEventListener('mouseup', onEnd);
    el.addEventListener('mouseleave', () => { if (el.classList.contains('active')) { el.classList.remove('active'); callEndpoint(releaseUrl); } });
  }

  setupHoldingEvent('btnF', '/F', '/SD');
  setupHoldingEvent('btnB', '/B', '/SD');
  setupHoldingEvent('btnL', '/L', '/CS');
  setupHoldingEvent('btnR', '/R', '/CS');

  document.getElementById('stopBtn').onclick = () => callEndpoint('/S');

  spdSlider.oninput = () => { spdVal.innerHTML = spdSlider.value + " / 255"; };
  spdSlider.onchange = () => { callEndpoint('/speed?val=' + spdSlider.value); };

  // Advanced Multi-Language Voice Commands implementation (Hindi / English Hybrid support)
  const voiceBtn = document.getElementById('voiceBtn');
  const Speech = window.SpeechRecognition || window.webkitSpeechRecognition;

  if (Speech) {
    const recognition = new Speech();
    recognition.lang = 'en-IN'; // Indian accent (Enables mixed Hindi recognition)
    recognition.continuous = false;

    voiceBtn.onclick = () => {
      recognition.start();
      voiceBtn.classList.add('listening');
      logger("Awaaz ka intezaar... (Speak forward, back, right, stop...)");
    };

    recognition.onresult = (event) => {
      const text = event.results[0][0].transcript.toLowerCase();
      logger('Suna: "' + text + '"');
      
      if (text.includes('forward') || text.includes('aage') || text.includes('chalo')) {
        callEndpoint('/F'); setTimeout(() => callEndpoint('/SD'), 2000);
      } else if (text.includes('back') || text.includes('peeche') || text.includes('piche')) {
        callEndpoint('/B'); setTimeout(() => callEndpoint('/SD'), 2000);
      } else if (text.includes('left') || text.includes('baaye') || text.includes('baye')) {
        callEndpoint('/L'); setTimeout(() => callEndpoint('/CS'), 1200);
      } else if (text.includes('right') || text.includes('daaye') || text.includes('daye')) {
        callEndpoint('/R'); setTimeout(() => callEndpoint('/CS'), 1200);
      } else if (text.includes('stop') || text.includes('ruko') || text.includes('ruk')) {
        callEndpoint('/S');
      }
    };

    recognition.onend = () => { voiceBtn.classList.remove('listening'); };
    recognition.onerror = (err) => { logger("Audio error: " + err.error); voiceBtn.classList.remove('listening'); };
  } else {
    voiceBtn.onclick = () => { logger("Aapke Browser/Phone par Web Speech API on nahi hai."); };
  }
</script>
</body>
</html>
)rawliteral";

// =========================================================================
// API HELPER & CORS (Future Android Studio App Integration)
// =========================================================================
void sendCorsResponse(String responseText) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", responseText);
}

// =========================================================================
// SETUP & CONFIGURATION
// =========================================================================
void setup() {
  Serial.begin(115200);

  // Pin outputs initialize karein
  pinMode(DRIVE_IN1, OUTPUT);
  pinMode(DRIVE_IN2, OUTPUT);
  pinMode(STEER_IN3, OUTPUT);
  pinMode(STEER_IN4, OUTPUT);
  pinMode(EEP_ENABLE, OUTPUT);

  // Initial safety limits
  digitalWrite(EEP_ENABLE, HIGH); // Hamesha chip ko awake (ON) rakhega
  
  // All motors state stop force initial
  analogWrite(DRIVE_IN1, 0);
  analogWrite(DRIVE_IN2, 0);
  analogWrite(STEER_IN3, 0);
  analogWrite(STEER_IN4, 0);

  // ESP32-C3 ka local WiFi AP mode shuru karein
  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi Hotspot Shuru Ho Gaya!");
  Serial.print("Go To: http://");
  Serial.println(WiFi.softAPIP());

  // Web routes
  server.on("/", []() {
    server.send_P(200, "text/html", index_html);
  });

  // Main Fwd/Bwd Motor Routing AIN1/AIN2
  server.on("/F", []() {
    analogWrite(DRIVE_IN1, driveSpeed);
    analogWrite(DRIVE_IN2, 0);
    sendCorsResponse("Aage Jaa Raha Hai");
  });

  server.on("/B", []() {
    analogWrite(DRIVE_IN1, 0);
    analogWrite(DRIVE_IN2, driveSpeed);
    sendCorsResponse("Peeche Jaa Raha Hai");
  });

  server.on("/SD", []() {
    analogWrite(DRIVE_IN1, 0);
    analogWrite(DRIVE_IN2, 0);
    sendCorsResponse("Bina Hinge Ke Momentum Blocked");
  });

  // L-Shape Tilt Motor Routing BIN1/BIN2
  server.on("/L", []() {
    analogWrite(STEER_IN3, steerSpeed);
    analogWrite(STEER_IN4, 0);
    sendCorsResponse("Left Pendulum Shift");
  });

  server.on("/R", []() {
    analogWrite(STEER_IN3, 0);
    analogWrite(STEER_IN4, steerSpeed);
    sendCorsResponse("Right Pendulum Shift");
  });

  server.on("/CS", []() {
    analogWrite(STEER_IN3, 0);
    analogWrite(STEER_IN4, 0);
    sendCorsResponse("Stabilized / Straights Active");
  });

  // Emergency global reset state
  server.on("/S", []() {
    analogWrite(DRIVE_IN1, 0);
    analogWrite(DRIVE_IN2, 0);
    analogWrite(STEER_IN3, 0);
    analogWrite(STEER_IN4, 0);
    sendCorsResponse("EVERYTHING SHUTDOWN");
  });

  // Speed value modulation API update
  server.on("/speed", []() {
    if (server.hasArg("val")) {
      driveSpeed = server.arg("val").toInt();
    }
    sendCorsResponse("Updated Speed to: " + String(driveSpeed));
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
