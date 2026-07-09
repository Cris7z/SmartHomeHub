#include "web_dashboard.h"

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "../app/ai_assistant.h"
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
<title>SmartHomeHub V1.1</title>
<style>
:root{
  color:#182126;
  background:#eef2f4;
  font-family:Inter,Arial,Helvetica,sans-serif;
  --ink:#182126;
  --muted:#62717a;
  --line:#d8e0e4;
  --panel:#ffffff;
  --panel2:#f7fafb;
  --dark:#142126;
  --teal:#008f8c;
  --blue:#2563eb;
  --amber:#d97706;
  --red:#c2410c;
  --green:#16803c;
}
*{box-sizing:border-box}
body{margin:0;background:#eef2f4;color:var(--ink)}
main{max-width:1060px;margin:0 auto;padding:18px}
.topbar{
  background:var(--dark);
  color:#fff;
  border-radius:8px;
  padding:18px;
  display:grid;
  grid-template-columns:minmax(0,1fr) auto;
  gap:14px;
  align-items:center;
  border:1px solid #24353b;
}
h1{font-size:26px;line-height:1.05;margin:0}
.subtitle{margin-top:6px;color:#b7c7cd;font-size:14px}
.topmeta{display:flex;gap:8px;flex-wrap:wrap;justify-content:flex-end}
.pill{border:1px solid #365057;border-radius:999px;padding:7px 10px;font-size:12px;color:#dce8eb;background:#1d3036}
.panel{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:14px}
.section{margin-top:12px}
.ai-panel{display:grid;grid-template-columns:minmax(0,1fr) 280px;gap:12px;align-items:stretch}
.ai-advice{border-left:5px solid var(--teal);min-height:112px}
.label{font-size:11px;letter-spacing:.06em;color:var(--muted);text-transform:uppercase;font-weight:800}
.value{font-size:26px;margin-top:7px;color:var(--blue);font-weight:800;line-height:1.12;overflow-wrap:anywhere}
.small{font-size:16px;font-weight:700;line-height:1.38}
.voicebar{display:grid;grid-template-columns:minmax(0,1fr) 78px 78px;gap:8px;margin-top:12px}
input{height:42px;border:1px solid var(--line);border-radius:6px;background:#fff;color:var(--ink);padding:0 12px;font-size:15px;min-width:0}
button{height:42px;border:0;border-radius:6px;background:var(--teal);color:#fff;font-weight:800;letter-spacing:.01em}
button.secondary{background:#34454c}
button.warn{background:var(--amber)}
button.danger{background:var(--red)}
.layout{display:grid;grid-template-columns:1.4fr .9fr;gap:12px;margin-top:12px}
.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}
.controls{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px}
.control-title{font-size:13px;font-weight:900;margin:0 0 10px;color:#34454c}
.events{margin-top:12px}
ul{margin:8px 0 0;padding:0;list-style:none;display:grid;gap:7px}
li{background:var(--panel2);border:1px solid var(--line);border-radius:6px;padding:8px 10px;color:#33444b;font-size:14px}
.ok{color:var(--green)}.warnText{color:var(--amber)}.bad{color:var(--red)}
.status-ok{border-color:#bfe3ce;background:#f1fbf5}
.status-warn{border-color:#f2d49b;background:#fff8eb}
.status-bad{border-color:#efb4a2;background:#fff3ef}
@media(max-width:780px){
  main{padding:12px}
  .topbar,.ai-panel,.layout{grid-template-columns:1fr}
  .topmeta{justify-content:flex-start}
  .grid{grid-template-columns:repeat(2,minmax(0,1fr))}
}
@media(max-width:500px){
  .grid,.controls{grid-template-columns:1fr}
  .voicebar{grid-template-columns:1fr 68px 68px}
  h1{font-size:22px}
}
</style>
</head>
<body>
<main>
<header class="topbar">
  <div>
    <h1>SmartHomeHub V1.1</h1>
    <div class="subtitle">AIoT 家庭安全管家</div>
  </div>
  <div class="topmeta">
    <div class="pill" id="netpill">NET --</div>
    <div class="pill" id="timepill">--:--</div>
    <div class="pill" id="relaypill">AI --</div>
  </div>
</header>

<section class="section ai-panel">
  <div class="panel ai-advice">
    <div class="label">AI Advice</div>
    <div class="value small" id="aiadvice">--</div>
    <div class="voicebar">
      <input id="xzask" placeholder="我出门了 / 我要睡觉了 / 现在安全吗" maxlength="80">
      <button class="secondary" id="talkBtn" onclick="listenXiaoZhi()">Mic</button>
      <button onclick="askXiaoZhi()">AI</button>
    </div>
  </div>
  <div class="panel">
    <div class="label">XiaoZhi</div>
    <div class="value" id="xiaozhi">--</div>
    <div class="label" style="margin-top:12px">Reply</div>
    <div class="value small" id="xzreply">--</div>
  </div>
</section>

<section class="layout">
  <div>
    <div class="grid">
      <div class="panel" id="roomcard"><div class="label">Room</div><div class="value" id="room">--</div></div>
      <div class="panel" id="seccard"><div class="label">Security</div><div class="value" id="sec">--</div></div>
      <div class="panel" id="guardcard"><div class="label">AI Guard</div><div class="value" id="guard">--</div></div>
      <div class="panel"><div class="label">Indoor</div><div class="value" id="indoor">--</div></div>
      <div class="panel"><div class="label">Outdoor</div><div class="value" id="outdoor">--</div></div>
      <div class="panel"><div class="label">Sunrise / Sunset</div><div class="value small" id="sun">--</div></div>
      <div class="panel"><div class="label">Noise</div><div class="value" id="noise">--</div></div>
      <div class="panel"><div class="label">Lamp</div><div class="value" id="lamp">--</div></div>
      <div class="panel"><div class="label">AC / IR</div><div class="value small" id="acir">--</div></div>
    </div>
    <div class="panel events">
      <div class="label">Recent Events</div>
      <ul id="events"></ul>
    </div>
  </div>
  <aside class="panel">
    <p class="control-title">Manual Controls</p>
    <div class="controls">
      <button onclick="cmd('security')">Security</button>
      <button onclick="cmd('lamp')">Lamp</button>
      <button onclick="cmd('ac')" class="warn">AC / IR</button>
      <button class="secondary" onclick="cmd('page')">Page</button>
      <button class="danger" onclick="cmd('clear')">Clear</button>
      <button class="secondary" onclick="cmd('xiaozhi')">XiaoZhi</button>
      <button class="secondary" onclick="cmd('home')">Home</button>
      <button class="secondary" onclick="cmd('away')">Away</button>
      <button class="secondary" onclick="cmd('night')">Night</button>
      <button class="secondary" onclick="refresh()">Refresh</button>
    </div>
    <div class="label" style="margin-top:14px">Heard</div>
    <div class="value small" id="xzprompt">--</div>
  </aside>
</section>
</main>
<script>
const $=id=>document.getElementById(id);
function setText(id,value){$(id).textContent=value;}
function setClass(id,name){$(id).className=name;}
function esc(text){return String(text).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
async function cmd(name){await fetch('/cmd?name='+encodeURIComponent(name));refresh();}
async function sendXiaoZhi(prompt){
 let url='/cmd?name=xiaozhi';
 if(prompt){url+='&prompt='+encodeURIComponent(prompt);}
 await fetch(url);
 await refresh();
}
async function askXiaoZhi(){
 const input=$('xzask');
 const text=input.value.trim();
 if(text){setText('xzprompt',text);}
 await sendXiaoZhi(text);
}
async function listenXiaoZhi(){
 await sendXiaoZhi('');
}
function yn(v){return v?'ON':'OFF'}
async function refresh(){
 const s=await (await fetch('/api/state')).json();
 setText('indoor',s.tempC.toFixed(1)+'C / '+s.humidity.toFixed(0)+'%');
 setText('outdoor',s.weatherReady?(s.location+' '+s.outdoorTempC.toFixed(1)+'C '+s.weather):'WAIT');
 setText('sun',s.weatherReady?(s.sunrise+' / '+s.sunset):'--');
 setText('room',s.presence?'OCCUPIED':'EMPTY');
 setClass('room','value '+(s.presence?'ok':'warnText'));
 setClass('roomcard','panel '+(s.presence?'status-ok':'status-warn'));
 setText('sec',s.securityArmed?'ARMED':'OFF');
 setClass('sec','value '+(s.alarm?'bad':(s.securityArmed?'warnText':'ok')));
 setClass('seccard','panel '+(s.alarm?'status-bad':(s.securityArmed?'status-warn':'status-ok')));
 setText('noise',s.micPercent+'%');
 setClass('noise','value '+(s.soundTriggered?'warnText':'ok'));
 setText('guard',s.aiRiskScore+' '+s.aiRisk);
 setClass('guard','value '+(s.aiRiskScore>=80?'bad':(s.aiRiskScore>=55?'warnText':'ok')));
 setClass('guardcard','panel '+(s.aiRiskScore>=80?'status-bad':(s.aiRiskScore>=55?'status-warn':'status-ok')));
 setText('lamp',s.lamp?'ON':'OFF');
 setText('acir',(s.acCooling?'COOLING':'STANDBY'));
 setText('netpill',s.wifiStaConnected?('NET '+s.ip):'NET OFFLINE');
 setText('timepill',s.time||'--:--');
 setText('relaypill',s.doubaoRelayConnected?'AI READY':'AI OFFLINE');
 setText('xiaozhi',s.xiaozhiPhase+(s.speakerPlaying?' / AUDIO':''));
 setClass('xiaozhi','value '+(s.xiaozhiPhase==='IDLE'||s.xiaozhiPhase==='DONE'?'ok':'warnText'));
 setText('xzprompt',s.xiaozhiPrompt||'--');
 setText('xzreply',s.xiaozhiReply||'--');
 setText('aiadvice',s.aiAdvice||'--');
 $('events').innerHTML=(s.events.length?s.events:['No events']).map(e=>'<li>'+esc(e)+'</li>').join('');
 return s;
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
  char aiAdvice[192];
  buildAiAdvice(aiAdvice, sizeof(aiAdvice));
  String json;
  json.reserve(1400);
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
  json += "\"aiAdvice\":" + jsonString(aiAdvice) + ",";
  json += "\"xiaozhiPhase\":\"" + String(state.xiaozhiStatusText) + "\",";
  json += "\"xiaozhiPrompt\":" + jsonString(state.xiaozhiPromptText) + ",";
  json += "\"xiaozhiReply\":" + jsonString(state.xiaozhiReplyText) + ",";
  json += "\"xiaozhiCloudConfigured\":" + boolJson(state.xiaozhiCloudConfigured) + ",";
  json += "\"doubaoRelayConfigured\":" + boolJson(state.doubaoRelayConfigured) + ",";
  json += "\"doubaoRelayConnected\":" + boolJson(state.doubaoRelayConnected) + ",";
  json += "\"doubaoSessionActive\":" + boolJson(state.doubaoSessionActive) + ",";
  json += "\"xiaozhiManualMode\":" + boolJson(state.xiaozhiManualMode) + ",";
  json += "\"xiaozhiError\":" + jsonString(state.xiaozhiErrorText) + ",";
  json += "\"speakerOk\":" + boolJson(state.i2sSpeakerOk) + ",";
  json += "\"speakerPlaying\":" + boolJson(state.speakerPlaying) + ",";
  json += "\"acCooling\":" + boolJson(state.acCooling) + ",";
  json += "\"lamp\":" + boolJson(state.alarm || (state.lampOverride ? state.manualLamp : state.presence)) + ",";
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
      char reply[160];
      applyAiTextCommand(prompt, "WEB AI", reply, sizeof(reply));
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
  dashboard.send(200, "application/json; charset=utf-8", stateJson());
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
