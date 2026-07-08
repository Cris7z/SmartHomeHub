#pragma once

#include <cstddef>

enum class XiaozhiCloudProvider {
  Local = 0,
  Anthropic,
  OpenAiChat,
  Unsupported
};

XiaozhiCloudProvider parseXiaozhiCloudProvider(const char *provider);
bool xiaozhiCloudConfigReady(const char *provider, const char *endpoint,
                             const char *token, const char *model);
const char *xiaozhiCloudPathForProvider(const char *provider);

bool formatXiaozhiHomeStatus(char *out, size_t outSize,
                             float tempC, float humidity,
                             bool presence, bool securityArmed,
                             const char *weatherText, const char *timeText);

bool buildXiaozhiAnthropicRequestJson(char *out, size_t outSize,
                                      const char *model, const char *prompt,
                                      const char *homeStatus);
bool parseXiaozhiAnthropicReply(const char *json, char *out, size_t outSize);

bool buildXiaozhiOpenAiChatRequestJson(char *out, size_t outSize,
                                       const char *model, const char *prompt,
                                       const char *homeStatus);
bool parseXiaozhiOpenAiChatReply(const char *json, char *out, size_t outSize);
