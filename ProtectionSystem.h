#pragma once

#include "DomainTypes.h"

class ProtectionSystem {
public:
  void begin();
  void serviceWatchdog();
  bool shouldEnterLockout(const BiasReading& reading, const CalibrationSettings& cal);
  bool canExitLockout(const BiasReading& reading, const CalibrationSettings& cal);
  bool isLocked() const { return locked_; }

private:
  bool locked_ = false;
};
