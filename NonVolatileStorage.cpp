#include "NonVolatileStorage.h"
#include "Config.h"
#include <EEPROM.h>

namespace {
  constexpr int CalibrationAddress = 0;
  constexpr int ProfileCountAddress = CalibrationAddress + sizeof(CalibrationSettings);
  constexpr int ProfilesAddress = ProfileCountAddress + sizeof(uint8_t);
  constexpr int ProfileChecksumAddress =
    ProfilesAddress + (sizeof(TubeProfile) * DeviceConfig::MaxTubeProfiles);

  uint8_t foldByte(uint8_t sum, uint8_t value) {
    return static_cast<uint8_t>(sum + value);
  }

  uint8_t foldUInt16(uint8_t sum, uint16_t value) {
    sum = foldByte(sum, static_cast<uint8_t>(value & 0xFF));
    sum = foldByte(sum, static_cast<uint8_t>((value >> 8) & 0xFF));
    return sum;
  }

  uint8_t checksumProfiles(const TubeProfile* profiles, uint8_t count) {
    uint8_t sum = count;

    for (uint8_t index = 0; index < count; ++index) {
      for (uint8_t charIndex = 0; charIndex < DeviceConfig::TubeNameLength; ++charIndex) {
        sum = foldByte(sum, static_cast<uint8_t>(profiles[index].label[charIndex]));
      }

      sum = foldByte(sum, profiles[index].maxDissipationWatts);
      sum = foldUInt16(sum, profiles[index].screenCurrentPermille);
    }

    return sum;
  }

  void copyLabel(char* target, const char* source) {
    uint8_t index = 0;

    for (; index < DeviceConfig::TubeNameLength - 1 && source[index] != '\0'; ++index) {
      target[index] = source[index];
    }

    for (; index < DeviceConfig::TubeNameLength; ++index) {
      target[index] = '\0';
    }
  }

  CalibrationSettings defaultCalibration() {
    CalibrationSettings settings;
    settings.signature = DeviceConfig::EepromSignature;
    settings.voltageScaleA_centi = 1000;
    settings.voltageScaleB_centi = 1000;
    settings.shuntA_milliohm = 1000;
    settings.shuntB_milliohm = 1000;
    settings.voltageLimit = 600;
    settings.checksum = 0;
    return settings;
  }

  uint8_t defaultProfiles(TubeProfile* profiles, uint8_t capacity) {
    if (capacity < 6) {
      return 0;
    }

    copyLabel(profiles[0].label, "6L6GC");
    profiles[0].maxDissipationWatts = 30;
    profiles[0].screenCurrentPermille = 55;

    copyLabel(profiles[1].label, "EL34");
    profiles[1].maxDissipationWatts = 25;
    profiles[1].screenCurrentPermille = 130;

    copyLabel(profiles[2].label, "6V6");
    profiles[2].maxDissipationWatts = 14;
    profiles[2].screenCurrentPermille = 45;

    copyLabel(profiles[3].label, "EL84");
    profiles[3].maxDissipationWatts = 12;
    profiles[3].screenCurrentPermille = 50;

    copyLabel(profiles[4].label, "KT88");
    profiles[4].maxDissipationWatts = 42;
    profiles[4].screenCurrentPermille = 60;

    copyLabel(profiles[5].label, "6550");
    profiles[5].maxDissipationWatts = 42;
    profiles[5].screenCurrentPermille = 60;

    for (uint8_t index = 6; index < capacity; ++index) {
      copyLabel(profiles[index].label, "");
      profiles[index].maxDissipationWatts = 0;
      profiles[index].screenCurrentPermille = 0;
    }

    return 6;
  }

  bool profileLooksValid(const TubeProfile& profile) {
    bool hasVisibleName = false;

    for (uint8_t index = 0; index < DeviceConfig::TubeNameLength; ++index) {
      const char c = profile.label[index];

      if (c == '\0') {
        break;
      }

      if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == ' ')) {
        return false;
      }

      if (c != ' ') {
        hasVisibleName = true;
      }
    }

    if (!hasVisibleName) {
      return false;
    }

    if (profile.maxDissipationWatts < 1 || profile.maxDissipationWatts > 99) {
      return false;
    }

    if (profile.screenCurrentPermille > 300) {
      return false;
    }

    return true;
  }
}

uint8_t NonVolatileStorage::checksumCalibration(const CalibrationSettings& settings) {
  uint8_t sum = 0;
  sum = foldUInt16(sum, settings.signature);
  sum = foldUInt16(sum, settings.voltageScaleA_centi);
  sum = foldUInt16(sum, settings.voltageScaleB_centi);
  sum = foldUInt16(sum, settings.shuntA_milliohm);
  sum = foldUInt16(sum, settings.shuntB_milliohm);
  sum = foldUInt16(sum, settings.voltageLimit);
  return sum;
}

bool NonVolatileStorage::calibrationIsValid(const CalibrationSettings& settings) {
  if (settings.signature != DeviceConfig::EepromSignature) return false;
  if (settings.checksum != checksumCalibration(settings)) return false;
  if (settings.voltageScaleA_centi < 500 || settings.voltageScaleA_centi > 2000) return false;
  if (settings.voltageScaleB_centi < 500 || settings.voltageScaleB_centi > 2000) return false;
  if (settings.shuntA_milliohm < 500 || settings.shuntA_milliohm > 5000) return false;
  if (settings.shuntB_milliohm < 500 || settings.shuntB_milliohm > 5000) return false;
  if (settings.voltageLimit < 300 || settings.voltageLimit > 800) return false;
  return true;
}

void NonVolatileStorage::beginWithDefaultsIfNeeded() {
  CalibrationSettings settings;
  EEPROM.get(CalibrationAddress, settings);

  if (!calibrationIsValid(settings)) {
    settings = defaultCalibration();
    settings.checksum = checksumCalibration(settings);
    EEPROM.put(CalibrationAddress, settings);

    TubeProfile profiles[DeviceConfig::MaxTubeProfiles];
    const uint8_t count = defaultProfiles(profiles, DeviceConfig::MaxTubeProfiles);
    saveProfiles(profiles, count);
    return;
  }

  TubeProfile profiles[DeviceConfig::MaxTubeProfiles];
  uint8_t count = 0;
  EEPROM.get(ProfileCountAddress, count);

  if (count == 0 || count > DeviceConfig::MaxTubeProfiles) {
    count = defaultProfiles(profiles, DeviceConfig::MaxTubeProfiles);
    saveProfiles(profiles, count);
    return;
  }

  for (uint8_t index = 0; index < count; ++index) {
    EEPROM.get(ProfilesAddress + (sizeof(TubeProfile) * index), profiles[index]);

    if (!profileLooksValid(profiles[index])) {
      count = defaultProfiles(profiles, DeviceConfig::MaxTubeProfiles);
      saveProfiles(profiles, count);
      return;
    }
  }

  uint8_t storedChecksum = 0;
  EEPROM.get(ProfileChecksumAddress, storedChecksum);

  if (storedChecksum != checksumProfiles(profiles, count)) {
    count = defaultProfiles(profiles, DeviceConfig::MaxTubeProfiles);
    saveProfiles(profiles, count);
  }
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
  bounded.signature = DeviceConfig::EepromSignature;

  if (bounded.voltageScaleA_centi < 500) bounded.voltageScaleA_centi = 500;
  if (bounded.voltageScaleA_centi > 2000) bounded.voltageScaleA_centi = 2000;
  if (bounded.voltageScaleB_centi < 500) bounded.voltageScaleB_centi = 500;
  if (bounded.voltageScaleB_centi > 2000) bounded.voltageScaleB_centi = 2000;
  if (bounded.shuntA_milliohm < 500) bounded.shuntA_milliohm = 500;
  if (bounded.shuntA_milliohm > 5000) bounded.shuntA_milliohm = 5000;
  if (bounded.shuntB_milliohm < 500) bounded.shuntB_milliohm = 500;
  if (bounded.shuntB_milliohm > 5000) bounded.shuntB_milliohm = 5000;
  if (bounded.voltageLimit < 300) bounded.voltageLimit = 300;
  if (bounded.voltageLimit > 800) bounded.voltageLimit = 800;

  bounded.checksum = checksumCalibration(bounded);
  EEPROM.put(CalibrationAddress, bounded);
}

uint8_t NonVolatileStorage::loadProfiles(TubeProfile* profiles, uint8_t capacity) {
  if (profiles == nullptr || capacity == 0) {
    return 0;
  }

  uint8_t count = 0;
  EEPROM.get(ProfileCountAddress, count);

  if (count == 0 || count > DeviceConfig::MaxTubeProfiles || count > capacity) {
    const uint8_t defaultCount = defaultProfiles(profiles, capacity);
    saveProfiles(profiles, defaultCount);
    return defaultCount;
  }

  for (uint8_t index = 0; index < count; ++index) {
    EEPROM.get(ProfilesAddress + (sizeof(TubeProfile) * index), profiles[index]);

    if (!profileLooksValid(profiles[index])) {
      const uint8_t defaultCount = defaultProfiles(profiles, capacity);
      saveProfiles(profiles, defaultCount);
      return defaultCount;
    }
  }

  uint8_t storedChecksum = 0;
  EEPROM.get(ProfileChecksumAddress, storedChecksum);

  if (storedChecksum != checksumProfiles(profiles, count)) {
    const uint8_t defaultCount = defaultProfiles(profiles, capacity);
    saveProfiles(profiles, defaultCount);
    return defaultCount;
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

  EEPROM.put(ProfileCountAddress, count);

  TubeProfile empty;
  copyLabel(empty.label, "");
  empty.maxDissipationWatts = 0;
  empty.screenCurrentPermille = 0;

  for (uint8_t index = 0; index < DeviceConfig::MaxTubeProfiles; ++index) {
    if (index < count && profileLooksValid(profiles[index])) {
      EEPROM.put(ProfilesAddress + (sizeof(TubeProfile) * index), profiles[index]);
    } else {
      EEPROM.put(ProfilesAddress + (sizeof(TubeProfile) * index), empty);
    }
  }

  const uint8_t profileChecksum = checksumProfiles(profiles, count);
  EEPROM.put(ProfileChecksumAddress, profileChecksum);
}
