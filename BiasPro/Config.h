#pragma once

// Arduino provides the fixed-width integer types via <Arduino.h> on the target;
// on a host build (e.g. the calculation tests) fall back to <cstdint> so this
// header - and everything that only needs the device constants/types - can be
// compiled without the Arduino toolchain.
#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <stdint.h>  // host builds: fixed-width types in the global namespace
#endif

namespace DeviceConfig {
constexpr uint8_t TftSclkPin = 13;
constexpr uint8_t TftMosiPin = 11;
constexpr uint8_t TftCsPin = 10;
constexpr uint8_t TftDcPin = 9;
constexpr uint8_t TftResetPin = 8;

constexpr uint8_t ButtonCenterPin = 7;
constexpr uint8_t ButtonLeftPin = 5;
constexpr uint8_t ButtonRightPin = 6;

constexpr uint8_t AdsAddress = 0x48;
constexpr uint16_t EepromSignature = 12357;
constexpr uint8_t MaxTubeProfiles = 10;
constexpr uint8_t TubeNameLength = 7;
constexpr uint8_t SampleWindow = 20;

constexpr float AdsMillivoltsPerBit = 0.0078125f;
constexpr float SafetyResetMarginVolts = 50.0f;
} // namespace DeviceConfig
