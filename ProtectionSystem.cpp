#include "ProtectionSystem.h"
#include "Config.h"

#ifdef BIASPRO_ENABLE_WDT
#include <avr/wdt.h>
#endif

namespace {
  float highestProbeVoltage(const BiasReading& reading) {
    return reading.voltsA > reading.voltsB ? reading.voltsA : reading.voltsB;
  }

  float releaseThreshold(const CalibrationSettings& cal) {
    if (cal.voltageLimit <= static_cast<uint16_t>(DeviceConfig::SafetyResetMarginVolts)) {
      return 0.0f;
    }

    return static_cast<float>(cal.voltageLimit) - DeviceConfig::SafetyResetMarginVolts;
  }
}

void ProtectionSystem::begin() {
  locked_ = false;

#ifdef BIASPRO_ENABLE_WDT
  wdt_enable(WDTO_4S);
#endif
}

void ProtectionSystem::serviceWatchdog() {
#ifdef BIASPRO_ENABLE_WDT
  wdt_reset();
#endif
}

bool ProtectionSystem::shouldEnterLockout(
  const BiasReading& reading,
  const CalibrationSettings& cal
) {
  const float limit = static_cast<float>(cal.voltageLimit);

  if (reading.voltsA > limit || reading.voltsB > limit) {
    locked_ = true;
    return true;
  }

  return locked_;
}

bool ProtectionSystem::canExitLockout(
  const BiasReading& reading,
  const CalibrationSettings& cal
) {
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
