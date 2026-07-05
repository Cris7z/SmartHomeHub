#pragma once

enum class HubCommand {
  ToggleSecurity,
  ToggleLamp,
  ToggleAc,
  NextDisplayPage,
  ClearAlarmSecurity,
  RunMacroHome,
  RunMacroAway,
  RunMacroNight
};

void applyHubCommand(HubCommand command, const char *source);
