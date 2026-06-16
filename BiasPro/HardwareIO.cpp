#include "HardwareIO.h"
#include "Config.h"
#include "MathCalculations.h"

bool HardwareIO::begin() {
  pinMode(DeviceConfig::ButtonCenterPin, INPUT_PULLUP);
  pinMode(DeviceConfig::ButtonLeftPin, INPUT_PULLUP);
  pinMode(DeviceConfig::ButtonRightPin, INPUT_PULLUP);

  if (!adc_.begin(DeviceConfig::AdsAddress)) {
    return false;
  }

  adc_.setGain(GAIN_SIXTEEN);
  adc_.setDataRate(RATE_ADS1115_860SPS);
  return true;
}

ButtonEvent HardwareIO::readButtonEvent() {
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

      if (now - lastButtonMillis_ < RepeatGuardMillis || longCenterSent_) {
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

  for (uint8_t index = 0; index < samples; ++index) {
    cathodeATotal += adc_.readADC_SingleEnded(0);
    plateATotal += adc_.readADC_SingleEnded(1);
    cathodeBTotal += adc_.readADC_SingleEnded(2);
    plateBTotal += adc_.readADC_SingleEnded(3);
  }

  RawAdcFrame frame;
  frame.cathodeA = static_cast<int16_t>(cathodeATotal / samples);
  frame.plateA = static_cast<int16_t>(plateATotal / samples);
  frame.cathodeB = static_cast<int16_t>(cathodeBTotal / samples);
  frame.plateB = static_cast<int16_t>(plateBTotal / samples);
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
