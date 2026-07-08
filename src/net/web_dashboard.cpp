#include "web_dashboard.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "../app/controls.h"
#include "../app/event_log.h"
#include "../app/hub_state.h"
#include "../app/xiaozhi_ai.h"
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
.buttons{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin:12px 0}
button{height:42px;border:0;border-radius:8px;background:#238636;color:#fff;font-weight:700}
button.secondary{background:#30363d}
.voicebar{display:grid;grid-template-columns:minmax(0,1fr) 86px 86px;gap:8px;margin:12px 0}
input{height:42px;border:1px solid #30363d;border-radius:8px;background:#0d1117;color:#e8edf2;padding:0 12px;font-size:16px}
.small{font-size:18px;line-height:1.25;word-break:break-word}
ul{margin:8px 0 0;padding-left:18px;color:#c9d1d9}
@media(max-width:560px){.grid,.buttons{grid-template-columns:1fr 1fr}.voicebar{grid-template-columns:1fr 72px 72px}}
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
<button class="secondary" onclick="cmd('home')">Home</button>
<button class="secondary" onclick="cmd('away')">Away</button>
<button class="secondary" onclick="cmd('night')">Night</button>
<button class="secondary" onclick="cmd('xiaozhi')">XiaoZhi</button>
</div>
<div class="voicebar">
<input id="xzask" placeholder="Ask XiaoZhi" maxlength="80">
<button class="secondary" id="talkBtn" onclick="listenXiaoZhi()">Talk</button>
<button onclick="askXiaoZhi()">Ask</button>
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
<div class="card"><div class="label">XiaoZhi</div><div class="value" id="xiaozhi">--</div></div>
<div class="card"><div class="label">Heard</div><div class="value small" id="xzprompt">--</div></div>
<div class="card"><div class="label">Reply</div><div class="value small" id="xzreply">--</div></div>
</section>
<div class="card" style="margin-top:10px"><div class="label">Recent Events</div><ul id="events"></ul></div>
</main>
<script>
async function cmd(name){await fetch('/cmd?name='+encodeURIComponent(name));refresh();}
async function sendXiaoZhi(prompt){
 let url='/cmd?name=xiaozhi';
 if(prompt){url+='&prompt='+encodeURIComponent(prompt);}
 await fetch(url);
 refresh();
}
async function askXiaoZhi(){
 const input=document.getElementById('xzask');
 const text=input.value.trim();
 if(text){xzprompt.textContent=text;}
 await sendXiaoZhi(text);
}
function listenXiaoZhi(){
 const Speech=window.SpeechRecognition||window.webkitSpeechRecognition;
 const button=document.getElementById('talkBtn');
 const input=document.getElementById('xzask');
 if(!Speech){button.textContent='Type';input.focus();return;}
 const rec=new Speech();
 rec.lang='zh-CN';
 rec.interimResults=false;
 rec.maxAlternatives=1;
 button.textContent='...';
 rec.onresult=function(e){
  const text=e.results[0][0].transcript.trim();
  input.value=text;
  xzprompt.textContent=text;
  sendXiaoZhi(text);
 };
 rec.onerror=function(){button.textContent='Talk';input.focus();};
 rec.onend=function(){if(button.textContent==='...')button.textContent='Talk';};
 rec.start();
}
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
 xiaozhi.textContent=s.xiaozhiPhase+(s.speakerPlaying?' / AUDIO':'');
 xiaozhi.className='value '+(s.xiaozhiPhase==='IDLE'?'ok':'warn');
 xzprompt.textContent=s.xiaozhiPrompt;
 xzreply.textContent=s.xiaozhiReply;
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

String jsonString(const char *value) {
  String out = "\"";
  const char *cursor = value ? value : "";
  while (*cursor) {
    const char c = *cursor++;
    if (c == '"' || c == '\\') {
      out += "\\";
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out += c;
    }
  }
  out += "\"";
  return out;
}

String stateJson() {
  String json;
  json.reserve(1120);
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
  json += "\"xiaozhiPhase\":\"" + String(state.xiaozhiStatusText) + "\",";
  json += "\"xiaozhiPrompt\":" + jsonString(state.xiaozhiPromptText) + ",";
  json += "\"xiaozhiReply\":" + jsonString(state.xiaozhiReplyText) + ",";
  json += "\"xiaozhiCloudConfigured\":" + boolJson(state.xiaozhiCloudConfigured) + ",";
  json += "\"speakerOk\":" + boolJson(state.i2sSpeakerOk) + ",";
  json += "\"speakerPlaying\":" + boolJson(state.speakerPlaying) + ",";
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
    json += jsonString(eventLogLine(i));
  }
  json += "]}";
  return json;
}

bool runDashboardCommand(const String &command, const String &prompt) {
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
  } else if (command == "home" || command == "macro_home") {
    applyHubCommand(HubCommand::RunMacroHome, "WEB");
  } else if (command == "away" || command == "macro_away") {
    applyHubCommand(HubCommand::RunMacroAway, "WEB");
  } else if (command == "night" || command == "macro_night") {
    applyHubCommand(HubCommand::RunMacroNight, "WEB");
  } else if (command == "xiaozhi" || command == "ai" || command == "voice") {
    if (prompt.length() > 0) {
      triggerXiaozhiAiWithPrompt("WEB speech", prompt.c_str());
    } else {
      applyHubCommand(HubCommand::TriggerXiaozhi, "WEB");
    }
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
  String prompt = dashboard.arg("prompt");
  prompt.trim();
  if (prompt.length() > 120) {
    prompt.remove(120);
  }
  dashboard.send(runDashboardCommand(command, prompt) ? 200 : 400, "text/plain", command);
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
