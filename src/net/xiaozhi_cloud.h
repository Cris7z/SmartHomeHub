#pragma once

#include <cstddef>

bool isXiaozhiCloudConfigured();
bool requestXiaozhiCloudReply(char *reply, size_t replySize,
                              const char *prompt,
                              float tempC, float humidity,
                              bool presence, bool securityArmed,
                              const char *weatherText, const char *timeText);
