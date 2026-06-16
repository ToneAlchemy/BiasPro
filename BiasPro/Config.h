#pragma once

#define BIASPRO_ENABLE_WDT

#include <Arduino.h>

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
