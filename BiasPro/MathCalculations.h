#pragma once

#include "DomainTypes.h"

namespace MathCalculations {
  float adcCountToMillivolts(int16_t count);
  ProbeStatus classifyProbe(int16_t cathodeCount, int16_t plateCount);
  BiasReading computeBias(
    const RawAdcFrame& raw,
    const CalibrationSettings& cal,
    const TubeProfile& tube
  );
}
