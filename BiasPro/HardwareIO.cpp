#include "HardwareIO.h"
#include "Config.h"
#include "MathCalculations.h"
#include <Wire.h>


bool HardwareIO::begin() {
  pinMode(DeviceConfig::ButtonCenterPin, INPUT_PULLUP);
  pinMode(DeviceConfig::ButtonLeftPin, INPUT_PULLUP);
  pinMode(DeviceConfig::ButtonRightPin, INPUT_PULLUP);

  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif
  Wire.beginTransmission(DeviceConfig::AdsAddress);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  return true;
}

ButtonEvent HardwareIO::readButtonEvent() {
  if (pendingEvent_ != ButtonEvent::None) {
    ButtonEvent e = pendingEvent_;
    pendingEvent_ = ButtonEvent::None;
    return e;
  }

  const uint32_t now = millis();
  constexpr uint32_t DebounceMillis = 25UL;
  constexpr uint32_t RepeatGuardMillis = 60UL;
  constexpr uint32_t LongPressMillis = 900UL;
  constexpr uint8_t LeftMask = 0x01;
  constexpr uint8_t RightMask = 0x02;
  constexpr uint8_t CenterMask = 0x04;

  uint8_t rawMask = 0;
  if (digitalRead(DeviceConfig::ButtonLeftPin) == LOW) rawMask |= LeftMask;
  if (digitalRead(DeviceConfig::ButtonRightPin) == LOW) rawMask |= RightMask;
  if (digitalRead(DeviceConfig::ButtonCenterPin) == LOW) rawMask |= CenterMask;

  if (rawMask != lastRawMask_) {
    lastRawMask_ = rawMask;
    lastRawChangeMillis_ = now;
    return ButtonEvent::None;
  }

  if (now - lastRawChangeMillis_ < DebounceMillis) {
    return ButtonEvent::None;
  }

  if (rawMask != stableMask_) {
    stableMask_ = rawMask;

    if (stableMask_ != 0 && !pressActive_) {
      pressActive_ = true;
      longCenterSent_ = false;
      pressedMask_ = stableMask_;
      pressStartMillis_ = now;
      return ButtonEvent::None;
    }

    if (stableMask_ == 0 && pressActive_) {
      pressActive_ = false;

      uint32_t guardMillis = ((pressedMask_ & CenterMask) != 0) ? 250UL : RepeatGuardMillis;

      if (now - lastButtonMillis_ < guardMillis || longCenterSent_) {
        return ButtonEvent::None;
      }

      lastButtonMillis_ = now;

      if ((pressedMask_ & CenterMask) != 0) return ButtonEvent::Center;
      if (pressedMask_ == LeftMask) return ButtonEvent::Left;
      if (pressedMask_ == RightMask) return ButtonEvent::Right;
    }

    return ButtonEvent::None;
  }

  if (
    pressActive_ &&
    !longCenterSent_ &&
    (stableMask_ & CenterMask) != 0 &&
    now - pressStartMillis_ >= LongPressMillis
  ) {
    longCenterSent_ = true;
    lastButtonMillis_ = now;
    return ButtonEvent::LongCenter;
  }

  return ButtonEvent::None;
}

RawAdcFrame HardwareIO::readAdcFrame(uint8_t samples) {
  if (samples == 0) {
    samples = 1;
  }

  int32_t cathodeATotal = 0;
  int32_t plateATotal = 0;
  int32_t cathodeBTotal = 0;
  int32_t plateBTotal = 0;
  bool frameValid = true;

  for (uint8_t index = 0; index < samples; ++index) {
    for (uint16_t i = 0; i < 4; ++i) {
      uint16_t config = 0x8000 | ((4 + i) << 12) | 0x0A00 | 0x0100 | 0x00E3;

      Wire.beginTransmission(DeviceConfig::AdsAddress);
      Wire.write(0x01);
      Wire.write((uint8_t)(config >> 8));
      Wire.write((uint8_t)(config & 0xFF));
      if (Wire.endTransmission() != 0) {
        frameValid = false;
      }

      delay(2);

      ButtonEvent e = readButtonEvent();
      if (e != ButtonEvent::None && pendingEvent_ == ButtonEvent::None) {
        pendingEvent_ = e;
      }

      Wire.beginTransmission(DeviceConfig::AdsAddress);
      Wire.write(0x00);
      if (Wire.endTransmission() != 0) {
        frameValid = false;
      }
      Wire.requestFrom((uint8_t)DeviceConfig::AdsAddress, (uint8_t)2);

      int16_t val = 0;
      if (Wire.available() == 2) {
          const uint8_t hi = Wire.read();
          const uint8_t lo = Wire.read();
          val = static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
      } else {
          frameValid = false;
      }

      if (i == 0) cathodeATotal += val;
      else if (i == 1) plateATotal += val;
      else if (i == 2) cathodeBTotal += val;
      else if (i == 3) plateBTotal += val;
    }
  }

  RawAdcFrame frame;
  frame.cathodeA = static_cast<int16_t>(cathodeATotal / samples);
  frame.plateA = static_cast<int16_t>(plateATotal / samples);
  frame.cathodeB = static_cast<int16_t>(cathodeBTotal / samples);
  frame.plateB = static_cast<int16_t>(plateBTotal / samples);
  frame.valid = frameValid;
  return frame;
}

SensorTelemetryFrame HardwareIO::readSensorTelemetry(uint8_t samples) {
  SensorTelemetryFrame telemetry;
  telemetry.counts = readAdcFrame(samples);

  telemetry.cathodeA_mV = MathCalculations::adcCountToMillivolts(telemetry.counts.cathodeA);
  telemetry.plateA_mV = MathCalculations::adcCountToMillivolts(telemetry.counts.plateA);
  telemetry.cathodeB_mV = MathCalculations::adcCountToMillivolts(telemetry.counts.cathodeB);
  telemetry.plateB_mV = MathCalculations::adcCountToMillivolts(telemetry.counts.plateB);

  telemetry.probeA = MathCalculations::classifyProbe(
    telemetry.counts.cathodeA,
    telemetry.counts.plateA
  );

  telemetry.probeB = MathCalculations::classifyProbe(
    telemetry.counts.cathodeB,
    telemetry.counts.plateB
  );

  return telemetry;
}
