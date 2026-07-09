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
  TriggerXiaozhi,
  ToggleXiaozhi,
  SetSecurityOn,
  SetSecurityOff,
  SetLampOn,
  SetLampOff,
  SetAcOn,
  SetAcOff
};

void applyHubCommand(HubCommand command, const char *source);
