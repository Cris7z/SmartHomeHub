#pragma once

#include <Arduino.h>

void logHubEvent(const char *message);
const char *eventLogLine(uint8_t newestIndex);
