#pragma once

#include <Arduino.h>
#include "DomainTypes.h"

class HardwareIO {
public:
  bool begin();
  ButtonEvent readButtonEvent();
  RawAdcFrame readAdcFrame(uint8_t samples);
  SensorTelemetryFrame readSensorTelemetry(uint8_t samples);

private:
  uint32_t lastButtonMillis_ = 0;
  uint32_t lastRawChangeMillis_ = 0;
  uint32_t pressStartMillis_ = 0;
  uint8_t lastRawMask_ = 0;
  uint8_t stableMask_ = 0;
  uint8_t pressedMask_ = 0;
  bool pressActive_ = false;
  bool longCenterSent_ = false;
  ButtonEvent pendingEvent_ = ButtonEvent::None;
};
