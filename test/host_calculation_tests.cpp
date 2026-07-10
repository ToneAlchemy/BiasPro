// Host-side verification of the firmware's pure calculation and storage logic.
//
// These tests compile and link the ACTUAL firmware sources (MathCalculations,
// ProtectionSystem, StorageCore) rather than a transcription, so the logic under
// test can never silently drift from what ships. The firmware pulls in
// <Arduino.h> only when the ARDUINO macro is defined; on a host build the
// shared headers fall back to <cstdint>, so no Arduino toolchain is required.
//
// Build & run (any C++17 host compiler):
//   g++ -std=c++17 -I BiasPro test/host_calculation_tests.cpp \
//       BiasPro/MathCalculations.cpp BiasPro/ProtectionSystem.cpp \
//       BiasPro/StorageCore.cpp -o biaspro_host_tests
//   ./biaspro_host_tests        # expect: "host calculation tests passed"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "DomainTypes.h"
#include "MathCalculations.h"
#include "ProtectionSystem.h"
#include "StorageCore.h"

using namespace MathCalculations;

namespace {
  CalibrationSettings makeValidCalibration() {
    CalibrationSettings settings = defaultCalibration();
    settings.checksum = checksumCalibration(settings);
    return settings;
  }

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

    CalibrationSettings bounded = corrupt;
    clampCalibration(bounded);
    assert(bounded.signature == DeviceConfig::EepromSignature);
    assert(bounded.shuntA_milliohm == 500);
    assert(bounded.shuntB_milliohm == 1000);
    assert(calibrationIsValid(bounded));

    CalibrationSettings outOfRange = makeValidCalibration();
    outOfRange.voltageScaleA_centi = 1;
    outOfRange.voltageScaleB_centi = 9999;
    outOfRange.shuntB_milliohm = 6000;
    outOfRange.voltageLimit = 1200;

    bounded = outOfRange;
    clampCalibration(bounded);
    assert(bounded.voltageScaleA_centi == 500);
    assert(bounded.voltageScaleB_centi == 2000);
    assert(bounded.shuntB_milliohm == 5000);
    assert(bounded.voltageLimit == 800);
    assert(calibrationIsValid(bounded));
  }

  void testEepromProfileBoundaryRecovery() {
    TubeProfile defaults[DeviceConfig::MaxTubeProfiles] = {};
    const uint8_t defaultCount = defaultProfiles(defaults, DeviceConfig::MaxTubeProfiles);

    assert(defaultCount == 6);
    assert(std::strcmp(defaults[0].label, "6L6GC") == 0);
    assert(std::strcmp(defaults[1].label, "EL34") == 0);
    assert(profileLooksValid(defaults[0]));
    assert(profileLooksValid(defaults[5]));
    assert(!profileLooksValid(defaults[6]));

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

    const uint8_t validChecksum = checksumProfiles(defaults, defaultCount);
    TubeProfile work[DeviceConfig::MaxTubeProfiles];

    // A valid image round-trips unchanged.
    std::memcpy(work, defaults, sizeof(defaults));
    assert(recoverProfiles(work, defaultCount, validChecksum,
                           DeviceConfig::MaxTubeProfiles) == defaultCount);
    assert(std::strcmp(work[1].label, "EL34") == 0);

    // Zero stored count -> defaults.
    std::memcpy(work, defaults, sizeof(defaults));
    assert(recoverProfiles(work, 0, validChecksum,
                           DeviceConfig::MaxTubeProfiles) == 6);
    assert(std::strcmp(work[0].label, "6L6GC") == 0);

    // Count beyond the hardware limit -> defaults.
    std::memcpy(work, defaults, sizeof(defaults));
    assert(recoverProfiles(work, DeviceConfig::MaxTubeProfiles + 1, validChecksum,
                           DeviceConfig::MaxTubeProfiles) == 6);

    // Count beyond the caller's capacity -> defaults (buffer too small).
    assert(recoverProfiles(tooSmall, 6, validChecksum, 5) == 0);

    // An invalid profile inside the counted range -> defaults.
    std::memcpy(work, defaults, sizeof(defaults));
    work[0] = invalid;
    const uint8_t invalidChecksum = checksumProfiles(work, defaultCount);
    assert(recoverProfiles(work, defaultCount, invalidChecksum,
                           DeviceConfig::MaxTubeProfiles) == 6);
    assert(std::strcmp(work[1].label, "EL34") == 0);

    // A checksum mismatch -> defaults.
    std::memcpy(work, defaults, sizeof(defaults));
    assert(recoverProfiles(work, defaultCount,
                           static_cast<uint8_t>(validChecksum + 1),
                           DeviceConfig::MaxTubeProfiles) == 6);
    assert(std::strcmp(work[0].label, "6L6GC") == 0);

    // usedDefaults out-param reflects whether defaults were substituted.
    bool usedDefaults = true;
    std::memcpy(work, defaults, sizeof(defaults));
    assert(recoverProfiles(work, defaultCount, validChecksum,
                           DeviceConfig::MaxTubeProfiles, &usedDefaults) == defaultCount);
    assert(!usedDefaults);
    std::memcpy(work, defaults, sizeof(defaults));
    assert(recoverProfiles(work, 0, validChecksum,
                           DeviceConfig::MaxTubeProfiles, &usedDefaults) == 6);
    assert(usedDefaults);
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
