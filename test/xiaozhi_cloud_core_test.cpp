#include <cassert>
#include <cstring>

#include "../src/net/xiaozhi_cloud_core.h"

namespace {
void testCloudConfigRequiresProviderEndpointTokenAndModel() {
  assert(!xiaozhiCloudConfigReady("local", "http://example.test", "token", "model"));
  assert(!xiaozhiCloudConfigReady("anthropic", "", "token", "model"));
  assert(!xiaozhiCloudConfigReady("anthropic", "http://example.test", "", "model"));
  assert(!xiaozhiCloudConfigReady("anthropic", "http://example.test", "token", ""));
  assert(xiaozhiCloudConfigReady("anthropic", "http://example.test", "token", "kimi-k2.6"));
  assert(xiaozhiCloudConfigReady("DeepSeek", "https://api.deepseek.com", "token", "deepseek-v4-pro"));
}

void testProviderPathUsesAnthropicMessagesEndpoint() {
  assert(std::strcmp(xiaozhiCloudPathForProvider("anthropic"), "/v1/messages") == 0);
  assert(std::strcmp(xiaozhiCloudPathForProvider("DeepSeek"), "/chat/completions") == 0);
  assert(std::strcmp(xiaozhiCloudPathForProvider("local"), "") == 0);
}

void testAnthropicRequestContainsModelAndPrompt() {
  char payload[512] = {};
  const bool ok = buildXiaozhiAnthropicRequestJson(
      payload, sizeof(payload), "kimi-k2.6",
      "请用一句中文回答: \"现在家里怎么样?\"",
      "室温26.5C, 湿度55%, 有人, 安防关闭");

  assert(ok);
  assert(std::strstr(payload, "\"model\":\"kimi-k2.6\"") != nullptr);
  assert(std::strstr(payload, "室温26.5C") != nullptr);
  assert(std::strstr(payload, "\\\"现在家里怎么样?\\\"") != nullptr);
  assert(std::strstr(payload, "secret-token") == nullptr);
}

void testAnthropicReplyParserExtractsText() {
  const char *json =
      "{\"content\":[{\"type\":\"text\",\"text\":\"现在家里温度合适，安防已关闭。\"}]}";
  char reply[96] = {};

  assert(parseXiaozhiAnthropicReply(json, reply, sizeof(reply)));
  assert(std::strcmp(reply, "现在家里温度合适，安防已关闭。") == 0);
}

void testOpenAiChatRequestAndReply() {
  char payload[512] = {};
  const bool ok = buildXiaozhiOpenAiChatRequestJson(
      payload, sizeof(payload), "deepseek-v4-pro",
      "看看家里状态", "Indoor 31.5C, humidity 60%, presence occupied.");

  assert(ok);
  assert(std::strstr(payload, "\"model\":\"deepseek-v4-pro\"") != nullptr);
  assert(std::strstr(payload, "\"thinking\":{\"type\":\"disabled\"}") != nullptr);
  assert(std::strstr(payload, "\"role\":\"system\"") != nullptr);
  assert(std::strstr(payload, "看看家里状态") != nullptr);

  const char *json =
      "{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":\"家里有人，温湿度正常。\"}}]}";
  char reply[96] = {};
  assert(parseXiaozhiOpenAiChatReply(json, reply, sizeof(reply)));
  assert(std::strcmp(reply, "家里有人，温湿度正常。") == 0);

  char shortReply[16] = {};
  assert(parseXiaozhiOpenAiChatReply(json, shortReply, sizeof(shortReply)));
  assert(shortReply[0] != '\0');
}
}

int main() {
  testCloudConfigRequiresProviderEndpointTokenAndModel();
  testProviderPathUsesAnthropicMessagesEndpoint();
  testAnthropicRequestContainsModelAndPrompt();
  testAnthropicReplyParserExtractsText();
  testOpenAiChatRequestAndReply();
  return 0;
}
