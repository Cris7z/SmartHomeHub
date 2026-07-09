#include "ai_assistant.h"

#include <Arduino.h>

#include "controls.h"
#include "hub_state.h"

namespace {
bool containsAny(const String &text, const char *a, const char *b = nullptr, const char *c = nullptr) {
  return text.indexOf(a) >= 0 || (b && text.indexOf(b) >= 0) || (c && text.indexOf(c) >= 0);
}

void copyReply(char *reply, size_t replySize, const char *text) {
  if (!reply || replySize == 0) return;
  strlcpy(reply, text ? text : "", replySize);
}

void setAiTexts(const String &prompt, const char *reply) {
  strlcpy(state.xiaozhiPromptText, prompt.c_str(), sizeof(state.xiaozhiPromptText));
  strlcpy(state.xiaozhiReplyText, reply ? reply : "", sizeof(state.xiaozhiReplyText));
  state.xiaozhiErrorText[0] = '\0';
}
}

void buildAiAdvice(char *out, size_t outSize) {
  if (!out || outSize == 0) return;

  if (state.alarm) {
    snprintf(out, outSize,
             "当前风险较高：系统处于%s状态，检测到异常声音并已触发本地报警，建议立即查看现场。",
             state.securityArmed ? "安防" : "普通");
  } else if (state.aiRiskScore >= 55) {
    snprintf(out, outSize,
             "当前风险偏高：%s%s%s建议关注房间状态。",
             state.securityArmed ? "系统已布防，" : "",
             state.soundTriggered ? "声音波动较大，" : "",
             state.irReceived ? "检测到红外信号，" : "");
  } else if (state.aiRiskScore >= 30) {
    snprintf(out, outSize,
             "当前为中等风险：%s声音和传感器状态需要继续观察。",
             state.securityArmed ? "安防已开启，" : "");
  } else {
    snprintf(out, outSize,
             "当前风险较低，室内%s，声音正常，温湿度稳定，建议保持%s模式。",
             state.presence ? "有人" : "无人",
             state.securityArmed ? "安防" : "居家");
  }
}

void buildAiAlertText(char *out, size_t outSize) {
  if (!out || outSize == 0) return;

  snprintf(out, outSize,
           "SmartHomeHub 安全提醒：<br>%s 检测到异常声音。<br>"
           "当前处于%s模式，系统已触发灯带闪烁和蜂鸣器报警。<br>"
           "室内 %.1fC，湿度 %.0f%%，AI Guard 风险 %u/%s。<br>"
           "建议立即查看现场或联系附近人员确认。",
           state.timeReady ? state.timeText : "--:--",
           state.securityArmed ? "安防" : "普通",
           state.tempC, state.humidity, state.aiRiskScore, state.aiRiskText);
}

bool applyAiTextCommand(const String &prompt, const char *source, char *reply, size_t replySize) {
  String text = prompt;
  text.trim();
  text.toLowerCase();
  if (text.length() == 0) {
    copyReply(reply, replySize, "请输入要执行的场景指令。");
    return false;
  }

  const char *label = source ? source : "AI";
  const char *result = nullptr;

  if (containsAny(text, "我出门", "离家", "出门了")) {
    applyHubCommand(HubCommand::RunMacroAway, label);
    result = "已切换离家安防模式。";
  } else if (containsAny(text, "睡觉", "夜间", "晚安")) {
    applyHubCommand(HubCommand::RunMacroNight, label);
    result = "已切换夜间模式，保留安防监测。";
  } else if (containsAny(text, "回家", "我回来了", "到家")) {
    applyHubCommand(HubCommand::RunMacroHome, label);
    result = "已切换回家模式。";
  } else if (containsAny(text, "关闭安防", "取消安防", "撤防")) {
    applyHubCommand(HubCommand::SetSecurityOff, label);
    result = "已关闭强制安防。";
  } else if (containsAny(text, "打开安防", "开启安防", "布防")) {
    applyHubCommand(HubCommand::SetSecurityOn, label);
    result = "已进入安防模式，房间按无人状态监测。";
  } else if (containsAny(text, "关灯", "关闭灯", "关闭灯带")) {
    applyHubCommand(HubCommand::SetLampOff, label);
    result = "已关闭灯带。";
  } else if (containsAny(text, "开灯", "打开灯", "打开灯带")) {
    applyHubCommand(HubCommand::SetLampOn, label);
    result = "已为你打开灯带。";
  } else if (containsAny(text, "关空调", "关闭空调", "停止制冷")) {
    applyHubCommand(HubCommand::SetAcOff, label);
    result = "已关闭空调控制演示。";
  } else if (containsAny(text, "开空调", "打开空调", "有点热") || text.indexOf("太热") >= 0) {
    applyHubCommand(HubCommand::SetAcOn, label);
    result = "室内温度偏高，已开启空调控制演示。";
  } else if (containsAny(text, "清除报警", "解除报警", "复位")) {
    applyHubCommand(HubCommand::ClearAlarmSecurity, label);
    result = "已清除报警并恢复自动安防。";
  } else if (containsAny(text, "安全吗", "当前状态", "报告状态")) {
    buildAiAdvice(reply, replySize);
    setAiTexts(prompt, reply);
    return true;
  } else {
    result = "当前作品未接入该功能，可执行安防、灯光、空调演示、报警复位和场景模式。";
  }

  copyReply(reply, replySize, result);
  setAiTexts(prompt, result);
  return true;
}
