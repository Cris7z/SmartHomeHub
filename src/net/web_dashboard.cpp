#include "web_dashboard.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "../app/controls.h"
#include "../app/event_log.h"
#include "../app/hub_state.h"
#include "../io/mic_processing.h"

namespace {
WebServer dashboard(80);
bool dashboardStarted = false;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Lexin Smart Home</title>
<style>
:root{font-family:Arial,Helvetica,sans-serif;color:#e8edf2;background:#0d1117}
body{margin:0;padding:18px;background:#0d1117}
main{max-width:760px;margin:0 auto}
h1{font-size:24px;margin:0 0 12px;color:#fff}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}
.card{background:#161b22;border:1px solid #30363d;border-radius:8px;padding:14px}
.label{font-size:12px;color:#8b949e;text-transform:uppercase}
.value{font-size:26px;margin-top:6px;color:#58a6ff}
.ok{color:#3fb950}.warn{color:#f2cc60}.bad{color:#ff7b72}
.buttons{display:grid;grid-template-columns:repeat(5,1fr);gap:8px;margin:12px 0}
button{height:42px;border:0;border-radius:8px;background:#238636;color:#fff;font-weight:700}
button.secondary{background:#30363d}
ul{margin:8px 0 0;padding-left:18px;color:#c9d1d9}
@media(max-width:560px){.grid,.buttons{grid-template-columns:1fr 1fr}.buttons button:last-child{grid-column:span 2}}
</style>
</head>
<body>
<main>
<h1>Lexin Smart Home</h1>
<div class="buttons">
<button onclick="cmd('security')">Security</button>
<button onclick="cmd('lamp')">Lamp</button>
<button onclick="cmd('ac')">AC / IR</button>
<button class="secondary" onclick="cmd('page')">Page</button>
<button class="secondary" onclick="cmd('clear')">Clear</button>
</div>
<section class="grid">
<div class="card"><div class="label">Indoor</div><div class="value" id="indoor">--</div></div>
<div class="card"><div class="label">Outdoor</div><div class="value" id="outdoor">--</div></div>
<div class="card"><div class="label">Sun</div><div class="value" id="sun">--</div></div>
<div class="card"><div class="label">Room</div><div class="value" id="room">--</div></div>
<div class="card"><div class="label">Security</div><div class="value" id="sec">--</div></div>
<div class="card"><div class="label">AI Noise</div><div class="value" id="noise">--</div></div>
<div class="card"><div class="label">AI Guard</div><div class="value" id="guard">--</div></div>
<div class="card"><div class="label">Network</div><div class="value" id="net">--</div></div>
</section>
<div class="card" style="margin-top:10px"><div class="label">Recent Events</div><ul id="events"></ul></div>
</main>
<script>
async function cmd(name){await fetch('/cmd?name='+encodeURIComponent(name));refresh();}
function yn(v){return v?'ON':'OFF'}
async function refresh(){
 const s=await (await fetch('/api/state')).json();
 indoor.textContent=s.tempC.toFixed(1)+'C / '+s.humidity.toFixed(0)+'%';
 outdoor.textContent=s.weatherReady?(s.location+' '+s.outdoorTempC.toFixed(1)+'C '+s.weather):'WAIT';
 sun.textContent=s.weatherReady?(s.sunrise+' / '+s.sunset):'--';
 room.textContent=s.presence?'OCCUPIED':'EMPTY';
 room.className='value '+(s.presence?'ok':'warn');
 sec.textContent=s.securityArmed?'ARMED':'OFF';
 sec.className='value '+(s.alarm?'bad':(s.securityArmed?'warn':'ok'));
 noise.textContent=s.micPercent+'%';
 noise.className='value '+(s.soundTriggered?'warn':'ok');
 guard.textContent=s.aiRiskScore+' '+s.aiRisk;
 guard.className='value '+(s.aiRiskScore>=80?'bad':(s.aiRiskScore>=55?'warn':'ok'));
 net.textContent=s.wifiStaConnected?s.ip:'OFFLINE';
 net.className='value '+(s.wifiStaConnected?'ok':'bad');
 events.innerHTML=(s.events.length?s.events:['No events']).map(e=>'<li>'+e+'</li>').join('');
}
setInterval(refresh,2000);refresh();
</script>
</body>
</html>
)rawliteral";

String boolJson(bool value) {
  return value ? "true" : "false";
}

String stateJson() {
  String json;
  json.reserve(940);
  json += "{";
  json += "\"tempC\":" + String(state.tempC, 1) + ",";
  json += "\"humidity\":" + String(state.humidity, 0) + ",";
  json += "\"presence\":" + boolJson(state.presence) + ",";
  json += "\"securityArmed\":" + boolJson(state.securityArmed) + ",";
  json += "\"alarm\":" + boolJson(state.alarm) + ",";
  json += "\"soundTriggered\":" + boolJson(state.soundTriggered) + ",";
  json += "\"micLevel\":" + String((long)state.micLevel) + ",";
  json += "\"micBaseline\":" + String((long)state.micBaseline) + ",";
  json += "\"micThreshold\":" + String((long)state.micThreshold) + ",";
  json += "\"micPercent\":" + String(noisePercentFor(state.micLevel, state.micThreshold)) + ",";
  json += "\"aiRiskScore\":" + String(state.aiRiskScore) + ",";
  json += "\"aiRisk\":\"" + String(state.aiRiskText) + "\",";
  json += "\"acCooling\":" + boolJson(state.acCooling) + ",";
  json += "\"lamp\":" + boolJson(state.lampOverride ? state.manualLamp : state.presence) + ",";
  json += "\"bleClientConnected\":" + boolJson(state.bleClientConnected) + ",";
  json += "\"wifiStaConnected\":" + boolJson(state.wifiStaConnected) + ",";
  json += "\"ip\":\"" + String(state.ipText) + "\",";
  json += "\"time\":\"" + String(state.timeText) + "\",";
  json += "\"weatherReady\":" + boolJson(state.weatherReady) + ",";
  json += "\"locationReady\":" + boolJson(state.locationReady) + ",";
  json += "\"location\":\"" + String(state.locationText) + "\",";
  json += "\"timezone\":\"" + String(state.timezoneText) + "\",";
  json += "\"outdoorTempC\":" + String(state.outdoorTempC, 1) + ",";
  json += "\"weather\":\"" + String(state.weatherText) + "\",";
  json += "\"windKph\":" + String(state.windKph, 1) + ",";
  json += "\"sunrise\":\"" + String(state.sunriseText) + "\",";
  json += "\"sunset\":\"" + String(state.sunsetText) + "\",";
  json += "\"events\":[";
  for (uint8_t i = 0; i < state.eventLogCount; i++) {
    if (i) json += ",";
    json += "\"" + String(eventLogLine(i)) + "\"";
  }
  json += "]}";
  return json;
}

bool runDashboardCommand(const String &command) {
  if (command == "security") {
    applyHubCommand(HubCommand::ToggleSecurity, "WEB");
  } else if (command == "lamp") {
    applyHubCommand(HubCommand::ToggleLamp, "WEB");
  } else if (command == "ac") {
    applyHubCommand(HubCommand::ToggleAc, "WEB");
  } else if (command == "page" || command == "mode") {
    applyHubCommand(HubCommand::NextDisplayPage, "WEB");
  } else if (command == "clear") {
    applyHubCommand(HubCommand::ClearAlarmSecurity, "WEB");
  } else {
    return false;
  }
  return true;
}

void handleIndex() {
  dashboard.send_P(200, "text/html", INDEX_HTML);
}

void handleApiState() {
  dashboard.send(200, "application/json", stateJson());
}

void handleCommand() {
  String command = dashboard.arg("name");
  command.trim();
  command.toLowerCase();
  dashboard.send(runDashboardCommand(command) ? 200 : 400, "text/plain", command);
}
}

void setupWebDashboard() {
  dashboard.on("/", HTTP_GET, handleIndex);
  dashboard.on("/api/state", HTTP_GET, handleApiState);
  dashboard.on("/cmd", HTTP_GET, handleCommand);
}

void updateWebDashboard() {
  if (!dashboardStarted && WiFi.status() == WL_CONNECTED) {
    dashboard.begin();
    dashboardStarted = true;
    Serial.printf("[WEB] dashboard ready: http://%s/\n", WiFi.localIP().toString().c_str());
    logHubEvent("WEB ready");
  }

  if (dashboardStarted) {
    dashboard.handleClient();
  }
}
