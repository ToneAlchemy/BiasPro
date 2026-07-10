#include "NonVolatileStorage.h"
#include "Config.h"
#include "StorageCore.h"
#include <EEPROM.h>

namespace {
  constexpr int CalibrationAddress = 0;
  constexpr int ProfileCountAddress = CalibrationAddress + sizeof(CalibrationSettings);
  constexpr int ProfilesAddress = ProfileCountAddress + sizeof(uint8_t);
  constexpr int ProfileChecksumAddress =
    ProfilesAddress + (sizeof(TubeProfile) * DeviceConfig::MaxTubeProfiles);
}

void NonVolatileStorage::beginWithDefaultsIfNeeded() {
  CalibrationSettings settings;
  EEPROM.get(CalibrationAddress, settings);

  if (!calibrationIsValid(settings)) {
    settings = defaultCalibration();
    clampCalibration(settings);
    EEPROM.put(CalibrationAddress, settings);

    // Corrupt or fresh chip: reset the profile block to defaults as well.
    TubeProfile profiles[DeviceConfig::MaxTubeProfiles];
    const uint8_t count = defaultProfiles(profiles, DeviceConfig::MaxTubeProfiles);
    saveProfiles(profiles, count);
    return;
  }

  // Calibration is sound; validate/repair the profile block. loadProfiles
  // persists defaults itself when the stored profiles are missing or corrupt.
  TubeProfile profiles[DeviceConfig::MaxTubeProfiles];
  loadProfiles(profiles, DeviceConfig::MaxTubeProfiles);
}

CalibrationSettings NonVolatileStorage::loadCalibration() {
  CalibrationSettings settings;
  EEPROM.get(CalibrationAddress, settings);

  if (!calibrationIsValid(settings)) {
    settings = defaultCalibration();
    settings.checksum = checksumCalibration(settings);
    EEPROM.put(CalibrationAddress, settings);
  }

  return settings;
}

void NonVolatileStorage::saveCalibration(const CalibrationSettings& settings) {
  CalibrationSettings bounded = settings;
  clampCalibration(bounded);
  EEPROM.put(CalibrationAddress, bounded);
}

uint8_t NonVolatileStorage::loadProfiles(TubeProfile* profiles, uint8_t capacity) {
  if (profiles == nullptr || capacity == 0) {
    return 0;
  }

  uint8_t storedCount = 0;
  EEPROM.get(ProfileCountAddress, storedCount);

  // Read the profiles the stored count claims (bounded by hardware limits and
  // the caller's buffer) so recoverProfiles can validate them.
  uint8_t readCount = storedCount;
  if (readCount > DeviceConfig::MaxTubeProfiles) readCount = DeviceConfig::MaxTubeProfiles;
  if (readCount > capacity) readCount = capacity;

  for (uint8_t index = 0; index < readCount; ++index) {
    EEPROM.get(ProfilesAddress + (sizeof(TubeProfile) * index), profiles[index]);
  }

  uint8_t storedChecksum = 0;
  EEPROM.get(ProfileChecksumAddress, storedChecksum);

  bool usedDefaults = false;
  const uint8_t count =
    recoverProfiles(profiles, storedCount, storedChecksum, capacity, &usedDefaults);

  if (usedDefaults) {
    saveProfiles(profiles, count);
  }

  return count;
}

void NonVolatileStorage::saveProfiles(const TubeProfile* profiles, uint8_t count) {
  if (profiles == nullptr) {
    return;
  }

  if (count > DeviceConfig::MaxTubeProfiles) {
    count = DeviceConfig::MaxTubeProfiles;
  }

  TubeProfile empty;
  copyLabel(empty.label, "");
  empty.maxDissipationWatts = 0;
  empty.screenCurrentPermille = 0;

  // Compact only the valid profiles into a contiguous run. Writing an invalid
  // record inside the counted range makes load-time validation fail and wipes
  // EVERY profile back to defaults; dropping the bad record preserves the good
  // ones. The stored checksum is then computed over exactly the bytes written,
  // so a load can never see a checksum/record mismatch.
  TubeProfile sanitized[DeviceConfig::MaxTubeProfiles];
  uint8_t validCount = 0;

  for (uint8_t index = 0; index < count; ++index) {
    if (profileLooksValid(profiles[index])) {
      sanitized[validCount] = profiles[index];
      ++validCount;
    }
  }

  EEPROM.put(ProfileCountAddress, validCount);

  for (uint8_t index = 0; index < DeviceConfig::MaxTubeProfiles; ++index) {
    if (index < validCount) {
      EEPROM.put(ProfilesAddress + (sizeof(TubeProfile) * index), sanitized[index]);
    } else {
      EEPROM.put(ProfilesAddress + (sizeof(TubeProfile) * index), empty);
    }
  }

  const uint8_t profileChecksum = checksumProfiles(sanitized, validCount);
  EEPROM.put(ProfileChecksumAddress, profileChecksum);
}
