#include "ApplicationController.h"
#include "Config.h"
#include "MathCalculations.h"

namespace {
  constexpr uint32_t StartupScreenMillis = 1200UL;
  constexpr uint32_t LiveRefreshMillis = 220UL;
  constexpr uint32_t TelemetryRefreshMillis = 350UL;

  void clampSelection(uint8_t& selected, uint8_t count) {
    if (count == 0) {
      selected = 0;
    } else if (selected >= count) {
      selected = count - 1;
    }
  }

  void adjustCalibration(CalibrationSettings& settings, uint8_t field, ButtonEvent event) {
    const bool increase = event == ButtonEvent::Right;
    const bool decrease = event == ButtonEvent::Left;

    if (!increase && !decrease) {
      return;
    }

    const int8_t direction = increase ? 1 : -1;

    if (field == 0) {
      int16_t value = static_cast<int16_t>(settings.voltageScaleA_centi) + (direction * 5);
      if (value < 500) value = 500;
      if (value > 2000) value = 2000;
      settings.voltageScaleA_centi = static_cast<uint16_t>(value);
    } else if (field == 1) {
      int16_t value = static_cast<int16_t>(settings.voltageScaleB_centi) + (direction * 5);
      if (value < 500) value = 500;
      if (value > 2000) value = 2000;
      settings.voltageScaleB_centi = static_cast<uint16_t>(value);
    } else if (field == 2) {
      int16_t value = static_cast<int16_t>(settings.shuntA_milliohm) + (direction * 10);
      if (value < 500) value = 500;
      if (value > 5000) value = 5000;
      settings.shuntA_milliohm = static_cast<uint16_t>(value);
    } else if (field == 3) {
      int16_t value = static_cast<int16_t>(settings.shuntB_milliohm) + (direction * 10);
      if (value < 500) value = 500;
      if (value > 5000) value = 5000;
      settings.shuntB_milliohm = static_cast<uint16_t>(value);
    } else {
      int16_t value = static_cast<int16_t>(settings.voltageLimit) + (direction * 5);
      if (value < 300) value = 300;
      if (value > 800) value = 800;
      settings.voltageLimit = static_cast<uint16_t>(value);
    }
  }
}

void ApplicationController::begin() {
  display_.begin();
  display_.showStartup();

  storage_.beginWithDefaultsIfNeeded();
  calibration_ = storage_.loadCalibration();
  profileCount_ = storage_.loadProfiles(profiles_, DeviceConfig::MaxTubeProfiles);
  clampSelection(selectedProfile_, profileCount_);

  hardwareReady_ = hardware_.begin();
  protection_.begin();

  activeScreen_ = ScreenId::TubeSelect;
  startupUntilMillis_ = millis() + StartupScreenMillis;
  lastRefreshMillis_ = 0;
  screenNeedsPaint_ = true;
}

void ApplicationController::tick() {
  protection_.serviceWatchdog();

  const uint32_t now = millis();
  

  if (startupUntilMillis_ != 0 && static_cast<int32_t>(now - startupUntilMillis_) < 0) {
    return;
  }

  if (startupUntilMillis_ != 0) {
    startupUntilMillis_ = 0;
    screenNeedsPaint_ = true;

    // Jump to calibration if Left button is held during startup
    if (digitalRead(DeviceConfig::ButtonLeftPin) == LOW) {
      activeScreen_ = ScreenId::Calibration;
      calibrationField_ = 0;
      return;
    }
  }

  if (!hardwareReady_) {
    if (screenNeedsPaint_) {
      display_.drawHardwareFault();
      screenNeedsPaint_ = false;
    }
    return;
  }

  const ButtonEvent event = hardware_.readButtonEvent();

  if (activeScreen_ == ScreenId::TubeSelect) {
    if (event == ButtonEvent::Left) {
      selectedProfile_ = selectedProfile_ == 0 ? profileCount_ : selectedProfile_ - 1;
      if (screenNeedsPaint_) {
        display_.drawTubeSelection(profiles_, profileCount_, selectedProfile_);
        screenNeedsPaint_ = false;
      } else {
        display_.updateTubeSelection(profiles_, profileCount_, selectedProfile_);
      }
    } else if (event == ButtonEvent::Right) {
      selectedProfile_ = selectedProfile_ + 1;
      if (selectedProfile_ > profileCount_) selectedProfile_ = 0;
      if (screenNeedsPaint_) {
        display_.drawTubeSelection(profiles_, profileCount_, selectedProfile_);
        screenNeedsPaint_ = false;
      } else {
        display_.updateTubeSelection(profiles_, profileCount_, selectedProfile_);
      }
    } else if (event == ButtonEvent::Center) {
      if (selectedProfile_ == profileCount_) {
        activeScreen_ = ScreenId::SensorTelemetry;
      } else {
        activeScreen_ = ScreenId::LiveBias;
      }
      screenNeedsPaint_ = true;
      lastRefreshMillis_ = 0;
      return;
    } else if (event == ButtonEvent::LongCenter) {
      if (selectedProfile_ < profileCount_) {
        activeScreen_ = ScreenId::ProfileManager;
        screenNeedsPaint_ = true;
        return;
      }
    }

    if (screenNeedsPaint_) {
      display_.drawTubeSelection(profiles_, profileCount_, selectedProfile_);
      screenNeedsPaint_ = false;
    }

    return;
  }

  if (activeScreen_ == ScreenId::LiveBias) {
    if (event == ButtonEvent::Center) {
      activeScreen_ = ScreenId::TubeSelect;
      screenNeedsPaint_ = true;
      return;
    }

    if ((now - lastRefreshMillis_ >= LiveRefreshMillis || screenNeedsPaint_) && profileCount_ > 0) {
      if (screenNeedsPaint_) {
        display_.drawLiveBiasFrame(profiles_[selectedProfile_]);
      }

      RawAdcFrame raw = hardware_.readAdcFrame(DeviceConfig::SampleWindow);
      BiasReading reading = MathCalculations::computeBias(
        raw,
        calibration_,
        profiles_[selectedProfile_]
      );

      if (protection_.shouldEnterLockout(reading, calibration_)) {
        activeScreen_ = ScreenId::FaultLockout;
        display_.drawVoltageLockout(reading.voltsA, reading.voltsB, calibration_.voltageLimit);
        lastRefreshMillis_ = now;
        screenNeedsPaint_ = false;
        return;
      }

      display_.updateLiveBiasValues(reading);
      lastRefreshMillis_ = now;
      screenNeedsPaint_ = false;
    }

    return;
  }

  if (activeScreen_ == ScreenId::FaultLockout) {
    if (event == ButtonEvent::LongCenter) {
      activeScreen_ = ScreenId::SensorTelemetry;
      screenNeedsPaint_ = true;
      lastRefreshMillis_ = 0;
      return;
    }

    if (now - lastRefreshMillis_ >= LiveRefreshMillis && profileCount_ > 0) {
      RawAdcFrame raw = hardware_.readAdcFrame(DeviceConfig::SampleWindow);
      BiasReading reading = MathCalculations::computeBias(
        raw,
        calibration_,
        profiles_[selectedProfile_]
      );

      if (protection_.canExitLockout(reading, calibration_)) {
        activeScreen_ = ScreenId::LiveBias;
        screenNeedsPaint_ = true;
      } else {
        display_.drawVoltageLockout(reading.voltsA, reading.voltsB, calibration_.voltageLimit);
      }

      lastRefreshMillis_ = now;
    } else if (screenNeedsPaint_) {
      display_.drawVoltageLockout(0.0f, 0.0f, calibration_.voltageLimit);
      screenNeedsPaint_ = false;
    }

    return;
  }

  if (activeScreen_ == ScreenId::SensorTelemetry) {
    if (event == ButtonEvent::Center) {
      if (protection_.isLocked()) {
        activeScreen_ = ScreenId::FaultLockout;
      } else {
        activeScreen_ = ScreenId::TubeSelect;
      }
      screenNeedsPaint_ = true;
      lastRefreshMillis_ = 0;
      return;
    }

    if (now - lastRefreshMillis_ >= TelemetryRefreshMillis || screenNeedsPaint_) {
      SensorTelemetryFrame telemetry = hardware_.readSensorTelemetry(DeviceConfig::SampleWindow);
      if (screenNeedsPaint_) {
        display_.drawSensorTelemetryFrame();
      }
      display_.updateSensorTelemetryValues(telemetry);
      lastRefreshMillis_ = now;
      screenNeedsPaint_ = false;
    }

    return;
  }

  if (activeScreen_ == ScreenId::ProfileManager) {
    if (event == ButtonEvent::Left && profileCount_ > 0) {
      selectedProfile_ = selectedProfile_ == 0 ? profileCount_ - 1 : selectedProfile_ - 1;
      if (screenNeedsPaint_) {
        display_.drawProfileManager(profiles_, profileCount_, selectedProfile_);
        screenNeedsPaint_ = false;
      } else {
        display_.updateProfileManagerSelection(profiles_, profileCount_, selectedProfile_);
      }
    } else if (event == ButtonEvent::Right && profileCount_ > 0) {
      selectedProfile_ = selectedProfile_ + 1;
      if (selectedProfile_ >= profileCount_) selectedProfile_ = 0;
      if (screenNeedsPaint_) {
        display_.drawProfileManager(profiles_, profileCount_, selectedProfile_);
        screenNeedsPaint_ = false;
      } else {
        display_.updateProfileManagerSelection(profiles_, profileCount_, selectedProfile_);
      }
    } else if (event == ButtonEvent::Center) {
      profileEditor_.open(selectedProfile_, profileCount_);
      activeScreen_ = ScreenId::ProfileEditor;
      screenNeedsPaint_ = true;
      return;
    } else if (event == ButtonEvent::LongCenter) {
      activeScreen_ = ScreenId::Calibration;
      calibrationField_ = 0;
      screenNeedsPaint_ = true;
      return;
    }

    if (screenNeedsPaint_) {
      display_.drawProfileManager(profiles_, profileCount_, selectedProfile_);
      screenNeedsPaint_ = false;
    }

    return;
  }

  if (activeScreen_ == ScreenId::ProfileEditor) {
    const ProfileEditorResult result = profileEditor_.handleInput(
      event,
      profiles_,
      profileCount_,
      DeviceConfig::MaxTubeProfiles
    );

    if (result == ProfileEditorResult::Saved || result == ProfileEditorResult::Deleted) {
      storage_.saveProfiles(profiles_, profileCount_);
      selectedProfile_ = profileEditor_.selectedIndex();
      clampSelection(selectedProfile_, profileCount_);
      activeScreen_ = ScreenId::ProfileManager;
      screenNeedsPaint_ = true;
      return;
    }

    if (result == ProfileEditorResult::Cancelled) {
      selectedProfile_ = profileEditor_.selectedIndex();
      clampSelection(selectedProfile_, profileCount_);
      activeScreen_ = ScreenId::ProfileManager;
      screenNeedsPaint_ = true;
      return;
    }

    if (event != ButtonEvent::None || screenNeedsPaint_) {
      profileEditor_.draw(display_.rawTft(), profiles_, profileCount_);
      screenNeedsPaint_ = false;
    }

    return;
  }

  if (activeScreen_ == ScreenId::Calibration) {
    if (event == ButtonEvent::Left || event == ButtonEvent::Right) {
      adjustCalibration(calibration_, calibrationField_, event);
      storage_.saveCalibration(calibration_);
      if (screenNeedsPaint_) {
        display_.drawCalibration(calibration_, calibrationField_);
        screenNeedsPaint_ = false;
      } else {
        display_.updateCalibrationValues(calibration_, calibrationField_);
      }
    } else if (event == ButtonEvent::Center) {
      calibrationField_ = calibrationField_ + 1;
      if (calibrationField_ > 4) {
        calibrationField_ = 0;
        activeScreen_ = ScreenId::ProfileManager;
        screenNeedsPaint_ = true;
        return;
      }
      display_.updateCalibrationValues(calibration_, calibrationField_);
    } else if (event == ButtonEvent::LongCenter) {
      storage_.saveCalibration(calibration_);
      activeScreen_ = ScreenId::TubeSelect;
      screenNeedsPaint_ = true;
      return;
    }

    if (screenNeedsPaint_) {
      display_.drawCalibration(calibration_, calibrationField_);
      screenNeedsPaint_ = false;
    }
  }
}
