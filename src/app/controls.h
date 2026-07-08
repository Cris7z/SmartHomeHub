#pragma once

enum class HubCommand {
  ToggleSecurity,
  ToggleLamp,
  ToggleAc,
  NextDisplayPage,
  ClearAlarmSecurity,
  RunMacroHome,
  RunMacroAway,
  RunMacroNight,
  TriggerXiaozhi
};

void applyHubCommand(HubCommand command, const char *source);
