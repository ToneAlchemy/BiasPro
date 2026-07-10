#pragma once

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include "Config.h"  // pulls in <cstdint> on host builds

enum class ButtonEvent : uint8_t {
  None,
  Left,
  Right,
  Center,
  LongCenter
};

enum class ScreenId : uint8_t {
  TubeSelect,
  LiveBias,
  SensorTelemetry,
  ProfileManager,
  ProfileEditor,
  Calibration,
  FaultLockout
};

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
  bool valid;  // false if any I2C transaction in the frame failed
};

struct SensorTelemetryFrame {
  RawAdcFrame counts;
  float cathodeA_mV;
  float plateA_mV;
  float cathodeB_mV;
  float plateB_mV;
  ProbeStatus probeA;
  ProbeStatus probeB;
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
