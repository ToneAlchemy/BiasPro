#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace DeviceConfig {
  constexpr uint16_t EepromSignature = 12357;
  constexpr uint8_t MaxTubeProfiles = 10;
  constexpr uint8_t TubeNameLength = 7;
  constexpr float AdsMillivoltsPerBit = 0.0078125f;
  constexpr float SafetyResetMarginVolts = 50.0f;
}

enum class ProbeStatus : uint8_t {
  Zero,
  Ok,
  Saturating
};

struct TubeProfile {
  char label[DeviceConfig::TubeNameLength];
  uint8_t maxDissipationWatts;
  uint16_t screenCurrentPermille;
};

struct CalibrationSettings {
  uint16_t signature;
  uint16_t voltageScaleA_centi;
  uint16_t voltageScaleB_centi;
  uint16_t shuntA_milliohm;
  uint16_t shuntB_milliohm;
  uint16_t voltageLimit;
  uint8_t checksum;
};

struct RawAdcFrame {
  int16_t cathodeA;
  int16_t plateA;
  int16_t cathodeB;
  int16_t plateB;
};

struct BiasReading {
  float voltsA;
  float milliampsA;
  float wattsA;
  float percentA;
  float voltsB;
  float milliampsB;
  float wattsB;
  float percentB;
  float currentDelta;
};

namespace {
  constexpr int16_t ZeroNoiseCounts = 10;
  constexpr int16_t SaturationCounts = 32000;

  float adcCountToMillivolts(int16_t count) {
    return static_cast<float>(count) * DeviceConfig::AdsMillivoltsPerBit;
  }

  float centiToFloat(uint16_t value) {
    return static_cast<float>(value) / 100.0f;
  }

  float milliohmToOhm(uint16_t value) {
    return static_cast<float>(value) / 1000.0f;
  }

  float permilleToRatio(uint16_t value) {
    return static_cast<float>(value) / 1000.0f;
  }

  float clampPositive(float value) {
    return value < 0.0f ? 0.0f : value;
  }

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

  uint8_t checksumCalibration(const CalibrationSettings& settings) {
    uint8_t sum = 0;
    sum = foldUInt16(sum, settings.signature);
    sum = foldUInt16(sum, settings.voltageScaleA_centi);
    sum = foldUInt16(sum, settings.voltageScaleB_centi);
    sum = foldUInt16(sum, settings.shuntA_milliohm);
    sum = foldUInt16(sum, settings.shuntB_milliohm);
    sum = foldUInt16(sum, settings.voltageLimit);
    return sum;
  }

  CalibrationSettings makeValidCalibration() {
    CalibrationSettings settings = defaultCalibration();
    settings.checksum = checksumCalibration(settings);
    return settings;
  }

  bool calibrationIsValid(const CalibrationSettings& settings) {
    if (settings.signature != DeviceConfig::EepromSignature) return false;
    if (settings.checksum != checksumCalibration(settings)) return false;
    if (settings.voltageScaleA_centi < 500 || settings.voltageScaleA_centi > 2000) return false;
    if (settings.voltageScaleB_centi < 500 || settings.voltageScaleB_centi > 2000) return false;
    if (settings.shuntA_milliohm < 500 || settings.shuntA_milliohm > 5000) return false;
    if (settings.shuntB_milliohm < 500 || settings.shuntB_milliohm > 5000) return false;
    if (settings.voltageLimit < 300 || settings.voltageLimit > 800) return false;
    return true;
  }

  CalibrationSettings boundedForSave(CalibrationSettings settings) {
    settings.signature = DeviceConfig::EepromSignature;

    if (settings.voltageScaleA_centi < 500) settings.voltageScaleA_centi = 500;
    if (settings.voltageScaleA_centi > 2000) settings.voltageScaleA_centi = 2000;
    if (settings.voltageScaleB_centi < 500) settings.voltageScaleB_centi = 500;
    if (settings.voltageScaleB_centi > 2000) settings.voltageScaleB_centi = 2000;
    if (settings.shuntA_milliohm < 500) settings.shuntA_milliohm = 500;
    if (settings.shuntA_milliohm > 5000) settings.shuntA_milliohm = 5000;
    if (settings.shuntB_milliohm < 500) settings.shuntB_milliohm = 500;
    if (settings.shuntB_milliohm > 5000) settings.shuntB_milliohm = 5000;
    if (settings.voltageLimit < 300) settings.voltageLimit = 300;
    if (settings.voltageLimit > 800) settings.voltageLimit = 800;

    settings.checksum = checksumCalibration(settings);
    return settings;
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

  struct ProfileEepromImage {
    uint8_t count;
    TubeProfile profiles[DeviceConfig::MaxTubeProfiles];
    uint8_t checksum;
  };

  ProfileEepromImage makeProfileImage(const TubeProfile* profiles, uint8_t count) {
    ProfileEepromImage image = {};
    image.count = count;

    for (uint8_t index = 0; index < DeviceConfig::MaxTubeProfiles; ++index) {
      image.profiles[index] = profiles[index];
    }

    image.checksum = checksumProfiles(image.profiles, count);
    return image;
  }

  uint8_t recoverProfilesFromImage(
    const ProfileEepromImage& image,
    TubeProfile* profiles,
    uint8_t capacity
  ) {
    const uint8_t count = image.count;

    if (count == 0 || count > DeviceConfig::MaxTubeProfiles || count > capacity) {
      return defaultProfiles(profiles, capacity);
    }

    for (uint8_t index = 0; index < count; ++index) {
      profiles[index] = image.profiles[index];

      if (!profileLooksValid(profiles[index])) {
        return defaultProfiles(profiles, capacity);
      }
    }

    if (image.checksum != checksumProfiles(profiles, count)) {
      return defaultProfiles(profiles, capacity);
    }

    return count;
  }

  ProbeStatus classifyProbe(int16_t cathodeCount, int16_t plateCount) {
    if (
      cathodeCount >= SaturationCounts ||
      cathodeCount <= -SaturationCounts ||
      plateCount >= SaturationCounts ||
      plateCount <= -SaturationCounts
    ) {
      return ProbeStatus::Saturating;
    }

    if (
      cathodeCount >= -ZeroNoiseCounts &&
      cathodeCount <= ZeroNoiseCounts &&
      plateCount >= -ZeroNoiseCounts &&
      plateCount <= ZeroNoiseCounts
    ) {
      return ProbeStatus::Zero;
    }

    return ProbeStatus::Ok;
  }

  BiasReading computeBias(
    const RawAdcFrame& raw,
    const CalibrationSettings& cal,
    const TubeProfile& tube
  ) {
    const float voltageScaleA = centiToFloat(cal.voltageScaleA_centi);
    const float voltageScaleB = centiToFloat(cal.voltageScaleB_centi);

    const float shuntA = milliohmToOhm(cal.shuntA_milliohm);
    const float shuntB = milliohmToOhm(cal.shuntB_milliohm);

    const float plateMillivoltsA = adcCountToMillivolts(raw.plateA);
    const float plateMillivoltsB = adcCountToMillivolts(raw.plateB);
    const float cathodeMillivoltsA = adcCountToMillivolts(raw.cathodeA);
    const float cathodeMillivoltsB = adcCountToMillivolts(raw.cathodeB);

    float voltsA = plateMillivoltsA * voltageScaleA;
    float voltsB = plateMillivoltsB * voltageScaleB;

    const float cathodeCurrentA = shuntA > 0.0f ? cathodeMillivoltsA / shuntA : 0.0f;
    const float cathodeCurrentB = shuntB > 0.0f ? cathodeMillivoltsB / shuntB : 0.0f;

    const float screenRatio = permilleToRatio(tube.screenCurrentPermille);
    float plateCurrentA = cathodeCurrentA * (1.0f - screenRatio);
    float plateCurrentB = cathodeCurrentB * (1.0f - screenRatio);

    voltsA = clampPositive(voltsA);
    voltsB = clampPositive(voltsB);
    plateCurrentA = clampPositive(plateCurrentA);
    plateCurrentB = clampPositive(plateCurrentB);

    BiasReading reading;
    reading.voltsA = voltsA;
    reading.milliampsA = plateCurrentA;
    reading.wattsA = (voltsA * plateCurrentA) / 1000.0f;

    reading.voltsB = voltsB;
    reading.milliampsB = plateCurrentB;
    reading.wattsB = (voltsB * plateCurrentB) / 1000.0f;

    if (tube.maxDissipationWatts > 0) {
      const float maxWatts = static_cast<float>(tube.maxDissipationWatts);
      reading.percentA = (reading.wattsA / maxWatts) * 100.0f;
      reading.percentB = (reading.wattsB / maxWatts) * 100.0f;
    } else {
      reading.percentA = 0.0f;
      reading.percentB = 0.0f;
    }

    const float delta = reading.milliampsA - reading.milliampsB;
    reading.currentDelta = delta < 0.0f ? -delta : delta;

    return reading;
  }

  float highestProbeVoltage(const BiasReading& reading) {
    return reading.voltsA > reading.voltsB ? reading.voltsA : reading.voltsB;
  }

  float releaseThreshold(const CalibrationSettings& cal) {
    if (cal.voltageLimit <= static_cast<uint16_t>(DeviceConfig::SafetyResetMarginVolts)) {
      return 0.0f;
    }

    return static_cast<float>(cal.voltageLimit) - DeviceConfig::SafetyResetMarginVolts;
  }

  class ProtectionSystem {
  public:
    void begin() {
      locked_ = false;
    }

    bool shouldEnterLockout(const BiasReading& reading, const CalibrationSettings& cal) {
      const float limit = static_cast<float>(cal.voltageLimit);

      if (reading.voltsA > limit || reading.voltsB > limit) {
        locked_ = true;
        return true;
      }

      return locked_;
    }

    bool canExitLockout(const BiasReading& reading, const CalibrationSettings& cal) {
      if (!locked_) {
        return true;
      }

      const float safeVoltage = releaseThreshold(cal);

      if (highestProbeVoltage(reading) < safeVoltage) {
        locked_ = false;
        return true;
      }

      return false;
    }

    bool isLocked() const {
      return locked_;
    }

  private:
    bool locked_ = false;
  };

  void assertNear(float actual, float expected, float tolerance = 0.02f) {
    assert(std::fabs(actual - expected) < tolerance);
  }

  void assertFiniteZero(float value) {
    assert(std::isfinite(value));
    assertNear(value, 0.0f);
  }

  BiasReading readingWithVolts(float voltsA, float voltsB) {
    BiasReading reading = {};
    reading.voltsA = voltsA;
    reading.voltsB = voltsB;
    return reading;
  }

  void testNominalBiasCalculation() {
    CalibrationSettings cal = makeValidCalibration();
    TubeProfile tube = {"EL34", 25, 130};
    RawAdcFrame raw = {5120, 5760, 5120, 5760};

    const BiasReading reading = computeBias(raw, cal, tube);

    assertNear(reading.voltsA, 450.0f);
    assertNear(reading.milliampsA, 34.8f);
    assertNear(reading.wattsA, 15.66f);
    assertNear(reading.percentA, 62.64f);
    assertNear(reading.currentDelta, 0.0f);
  }

  void testProbeFaultClassification() {
    assert(classifyProbe(0, 0) == ProbeStatus::Zero);
    assert(classifyProbe(10, -10) == ProbeStatus::Zero);
    assert(classifyProbe(11, 0) == ProbeStatus::Ok);
    assert(classifyProbe(0, 11) == ProbeStatus::Ok);
    assert(classifyProbe(32000, 0) == ProbeStatus::Saturating);
    assert(classifyProbe(0, -32000) == ProbeStatus::Saturating);
    assert(classifyProbe(-32000, 0) == ProbeStatus::Saturating);
    assert(classifyProbe(0, 32000) == ProbeStatus::Saturating);
    assert(classifyProbe(-32768, 0) == ProbeStatus::Saturating);
    assert(classifyProbe(0, -32768) == ProbeStatus::Saturating);
  }

  void testDivisionByZeroShuntsAreSafe() {
    CalibrationSettings cal = makeValidCalibration();
    cal.shuntA_milliohm = 0;
    cal.shuntB_milliohm = 0;

    TubeProfile tube = {"EL34", 25, 130};
    RawAdcFrame raw = {5120, 5760, 5120, 5760};

    const BiasReading reading = computeBias(raw, cal, tube);

    assertNear(reading.voltsA, 450.0f);
    assertNear(reading.voltsB, 450.0f);
    assertFiniteZero(reading.milliampsA);
    assertFiniteZero(reading.milliampsB);
    assertFiniteZero(reading.wattsA);
    assertFiniteZero(reading.wattsB);
    assertFiniteZero(reading.percentA);
    assertFiniteZero(reading.percentB);
    assertFiniteZero(reading.currentDelta);
  }

  void testZeroMaxDissipationIsSafe() {
    CalibrationSettings cal = makeValidCalibration();
    TubeProfile tube = {"TEST", 0, 130};
    RawAdcFrame raw = {5120, 5760, 5120, 5760};

    const BiasReading reading = computeBias(raw, cal, tube);

    assertNear(reading.wattsA, 15.66f);
    assertFiniteZero(reading.percentA);
    assertFiniteZero(reading.percentB);
  }

  void testNegativeReadingsClampToZero() {
    CalibrationSettings cal = makeValidCalibration();
    TubeProfile tube = {"EL34", 25, 130};
    RawAdcFrame raw = {-5120, -5760, -5120, -5760};

    const BiasReading reading = computeBias(raw, cal, tube);

    assertFiniteZero(reading.voltsA);
    assertFiniteZero(reading.voltsB);
    assertFiniteZero(reading.milliampsA);
    assertFiniteZero(reading.milliampsB);
    assertFiniteZero(reading.wattsA);
    assertFiniteZero(reading.wattsB);
    assertFiniteZero(reading.percentA);
    assertFiniteZero(reading.percentB);
  }

  void testEepromCalibrationBoundaryRecovery() {
    CalibrationSettings corrupt = makeValidCalibration();
    corrupt.shuntA_milliohm = 0;
    corrupt.checksum = checksumCalibration(corrupt);
    assert(!calibrationIsValid(corrupt));

    CalibrationSettings bounded = boundedForSave(corrupt);
    assert(bounded.signature == DeviceConfig::EepromSignature);
    assert(bounded.shuntA_milliohm == 500);
    assert(bounded.shuntB_milliohm == 1000);
    assert(calibrationIsValid(bounded));

    CalibrationSettings outOfRange = makeValidCalibration();
    outOfRange.voltageScaleA_centi = 1;
    outOfRange.voltageScaleB_centi = 9999;
    outOfRange.shuntB_milliohm = 6000;
    outOfRange.voltageLimit = 1200;

    bounded = boundedForSave(outOfRange);
    assert(bounded.voltageScaleA_centi == 500);
    assert(bounded.voltageScaleB_centi == 2000);
    assert(bounded.shuntB_milliohm == 5000);
    assert(bounded.voltageLimit == 800);
    assert(calibrationIsValid(bounded));
  }

  void testEepromProfileBoundaryRecovery() {
    TubeProfile profiles[DeviceConfig::MaxTubeProfiles] = {};
    const uint8_t recoveredCount = defaultProfiles(profiles, DeviceConfig::MaxTubeProfiles);

    assert(recoveredCount == 6);
    assert(std::strcmp(profiles[0].label, "6L6GC") == 0);
    assert(std::strcmp(profiles[1].label, "EL34") == 0);
    assert(profileLooksValid(profiles[0]));
    assert(profileLooksValid(profiles[5]));
    assert(!profileLooksValid(profiles[6]));

    TubeProfile tooSmall[5] = {};
    assert(defaultProfiles(tooSmall, 5) == 0);

    TubeProfile invalid = {};
    copyLabel(invalid.label, "bad!");
    invalid.maxDissipationWatts = 25;
    invalid.screenCurrentPermille = 130;
    assert(!profileLooksValid(invalid));

    TubeProfile excessiveScreenCurrent = {};
    copyLabel(excessiveScreenCurrent.label, "EL34");
    excessiveScreenCurrent.maxDissipationWatts = 25;
    excessiveScreenCurrent.screenCurrentPermille = 301;
    assert(!profileLooksValid(excessiveScreenCurrent));

    const ProfileEepromImage validImage = makeProfileImage(profiles, recoveredCount);
    TubeProfile recovered[DeviceConfig::MaxTubeProfiles] = {};
    assert(recoverProfilesFromImage(validImage, recovered, DeviceConfig::MaxTubeProfiles) == recoveredCount);
    assert(std::strcmp(recovered[1].label, "EL34") == 0);

    ProfileEepromImage zeroCountImage = validImage;
    zeroCountImage.count = 0;
    assert(recoverProfilesFromImage(zeroCountImage, recovered, DeviceConfig::MaxTubeProfiles) == 6);
    assert(std::strcmp(recovered[0].label, "6L6GC") == 0);

    ProfileEepromImage excessiveCountImage = validImage;
    excessiveCountImage.count = DeviceConfig::MaxTubeProfiles + 1;
    assert(recoverProfilesFromImage(excessiveCountImage, recovered, DeviceConfig::MaxTubeProfiles) == 6);

    ProfileEepromImage capacityOverflowImage = validImage;
    capacityOverflowImage.count = 6;
    assert(recoverProfilesFromImage(capacityOverflowImage, tooSmall, 5) == 0);

    ProfileEepromImage invalidProfileImage = validImage;
    invalidProfileImage.profiles[0] = invalid;
    invalidProfileImage.checksum = checksumProfiles(invalidProfileImage.profiles, invalidProfileImage.count);
    assert(recoverProfilesFromImage(invalidProfileImage, recovered, DeviceConfig::MaxTubeProfiles) == 6);
    assert(std::strcmp(recovered[1].label, "EL34") == 0);

    ProfileEepromImage checksumMismatchImage = validImage;
    checksumMismatchImage.checksum = static_cast<uint8_t>(checksumMismatchImage.checksum + 1);
    assert(recoverProfilesFromImage(checksumMismatchImage, recovered, DeviceConfig::MaxTubeProfiles) == 6);
    assert(std::strcmp(recovered[0].label, "6L6GC") == 0);
  }

  void testProtectionReleaseMargin() {
    CalibrationSettings cal = makeValidCalibration();
    ProtectionSystem protection;
    protection.begin();

    assert(!protection.isLocked());
    assert(!protection.shouldEnterLockout(readingWithVolts(600.0f, 600.0f), cal));
    assert(protection.shouldEnterLockout(readingWithVolts(601.0f, 500.0f), cal));
    assert(protection.isLocked());

    assert(!protection.canExitLockout(readingWithVolts(550.0f, 100.0f), cal));
    assert(!protection.canExitLockout(readingWithVolts(100.0f, 550.0f), cal));
    assert(protection.canExitLockout(readingWithVolts(549.9f, 100.0f), cal));
    assert(!protection.isLocked());
    assert(!protection.shouldEnterLockout(readingWithVolts(100.0f, 100.0f), cal));
  }

  void testProtectionLowLimitReleaseBranch() {
    CalibrationSettings cal = makeValidCalibration();
    cal.voltageLimit = 50;

    ProtectionSystem protection;
    protection.begin();

    assert(protection.shouldEnterLockout(readingWithVolts(51.0f, 0.0f), cal));
    assert(!protection.canExitLockout(readingWithVolts(0.0f, 0.0f), cal));
    assert(protection.canExitLockout(readingWithVolts(-0.1f, -0.1f), cal));
  }
}

int main() {
  testNominalBiasCalculation();
  testProbeFaultClassification();
  testDivisionByZeroShuntsAreSafe();
  testZeroMaxDissipationIsSafe();
  testNegativeReadingsClampToZero();
  testEepromCalibrationBoundaryRecovery();
  testEepromProfileBoundaryRecovery();
  testProtectionReleaseMargin();
  testProtectionLowLimitReleaseBranch();

  std::cout << "host calculation tests passed\n";
  return 0;
}
