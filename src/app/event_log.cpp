#include "event_log.h"

#include "hub_state.h"

void logHubEvent(const char *message) {
  if (!message || message[0] == '\0') return;

  char line[HUB_EVENT_TEXT_LEN];
  snprintf(line, sizeof(line), "%s %s", state.timeReady ? state.timeText : "--:--", message);
  strlcpy(state.eventLog[state.eventLogHead], line, sizeof(state.eventLog[state.eventLogHead]));
  state.eventLogHead = (state.eventLogHead + 1) % HUB_EVENT_LOG_COUNT;
  if (state.eventLogCount < HUB_EVENT_LOG_COUNT) {
    state.eventLogCount++;
  }
  Serial.printf("[EVENT] %s\n", line);
}
const char *eventLogLine(uint8_t newestIndex) {
  if (newestIndex >= state.eventLogCount) return "";
  const int newest = (state.eventLogHead + HUB_EVENT_LOG_COUNT - 1 - newestIndex) % HUB_EVENT_LOG_COUNT;
  return state.eventLog[newest];
}
