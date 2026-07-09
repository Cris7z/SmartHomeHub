#pragma once

#include <Arduino.h>
#include <cstddef>

void buildAiAdvice(char *out, size_t outSize);
void buildAiAlertText(char *out, size_t outSize);
bool applyAiTextCommand(const String &prompt, const char *source, char *reply, size_t replySize);
