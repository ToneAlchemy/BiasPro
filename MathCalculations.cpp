#include "MathCalculations.h"
#include "Config.h"

namespace {
  constexpr int16_t ZeroNoiseCounts = 10;
  constexpr int16_t SaturationCounts = 32000;

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
}

namespace MathCalculations {

float adcCountToMillivolts(int16_t count) {
  return static_cast<float>(count) * DeviceConfig::AdsMillivoltsPerBit;
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

}
