#pragma once

#include "HardwareIO.h"
#include "NonVolatileStorage.h"
#include "ProtectionSystem.h"
#include "DisplayManager.h"
#include "ProfileEditor.h"

class ApplicationController {
public:
  void begin();
  void tick();

private:
  HardwareIO hardware_;
  NonVolatileStorage storage_;
  ProtectionSystem protection_;
  DisplayManager display_;
  ProfileEditor profileEditor_;

  CalibrationSettings calibration_;
  TubeProfile profiles_[DeviceConfig::MaxTubeProfiles];
  uint8_t profileCount_ = 0;
  uint8_t selectedProfile_ = 0;
  uint8_t calibrationField_ = 0;
  ScreenId activeScreen_ = ScreenId::TubeSelect;
  uint32_t startupUntilMillis_ = 0;
  uint32_t lastRefreshMillis_ = 0;
  bool screenNeedsPaint_ = true;
  bool hardwareReady_ = false;
};
