#pragma once

#include "DomainTypes.h"

class NonVolatileStorage {
public:
  void beginWithDefaultsIfNeeded();
  CalibrationSettings loadCalibration();
  void saveCalibration(const CalibrationSettings& settings);
  uint8_t loadProfiles(TubeProfile* profiles, uint8_t capacity);
  void saveProfiles(const TubeProfile* profiles, uint8_t count);

private:
  uint8_t checksumCalibration(const CalibrationSettings& settings);
  bool calibrationIsValid(const CalibrationSettings& settings);
};
