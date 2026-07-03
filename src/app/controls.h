#pragma once

enum class HubCommand {
  ToggleSecurity,
  ToggleLamp,
  ToggleAc,
  NextDisplayPage,
  ClearAlarmSecurity
};

void applyHubCommand(HubCommand command, const char *source);
