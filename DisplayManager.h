#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "DomainTypes.h"

class DisplayManager {
public:
  DisplayManager();

  void begin();
  Adafruit_ST7735& rawTft();
  void showStartup();
  void drawTubeSelection(const TubeProfile* profiles, uint8_t count, uint8_t selectedIndex);
  void updateTubeSelection(const TubeProfile* profiles, uint8_t count, uint8_t selectedIndex);
  void drawLiveBiasFrame(const TubeProfile& tube);
  void updateLiveBiasValues(const BiasReading& reading);
  void drawSensorTelemetryFrame();
  void updateSensorTelemetryValues(const SensorTelemetryFrame& telemetry);
  void drawProfileManager(const TubeProfile* profiles, uint8_t count, uint8_t selectedIndex);
  void updateProfileManagerSelection(const TubeProfile* profiles, uint8_t count, uint8_t selectedIndex);
  void drawCalibration(const CalibrationSettings& settings, uint8_t selectedField);
  void updateCalibrationValues(const CalibrationSettings& settings, uint8_t selectedField);
  void drawVoltageLockout(float voltsA, float voltsB, uint16_t limit);
  void drawHardwareFault();

private:
  Adafruit_ST7735 tft_;
  bool liveValuesPrimed_ = false;
  int16_t liveVoltsA10_ = 0;
  int16_t liveMilliampsA10_ = 0;
  int16_t liveWattsA10_ = 0;
  int16_t livePercentA_ = 0;
  int16_t liveVoltsB10_ = 0;
  int16_t liveMilliampsB10_ = 0;
  int16_t liveWattsB10_ = 0;
  int16_t livePercentB_ = 0;
  int16_t liveDelta10_ = 0;
  bool calibrationValuesPrimed_ = false;
  uint8_t calibrationSelected_ = 255;

  void drawLiveTenth(int16_t value, int16_t& previous, int16_t x, int16_t y, uint16_t color);
  void drawLiveWhole(int16_t value, int16_t& previous, int16_t x, int16_t y, uint16_t color);
  void drawCalibrationRow(uint8_t fieldIndex, bool selected, uint16_t value);
};
