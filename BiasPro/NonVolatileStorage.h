#pragma once

#include "DomainTypes.h"
#include "StorageCore.h"  // pure checksum/validation/default/recovery logic

class NonVolatileStorage {
public:
  void beginWithDefaultsIfNeeded();
  CalibrationSettings loadCalibration();
  void saveCalibration(const CalibrationSettings& settings);
  uint8_t loadProfiles(TubeProfile* profiles, uint8_t capacity);
  void saveProfiles(const TubeProfile* profiles, uint8_t count);
};
