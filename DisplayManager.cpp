#include "DisplayManager.h"
#include "Config.h"

namespace {
constexpr uint16_t ColorPanel = 0x2104;
constexpr uint16_t ColorGood = ST7735_GREEN;
constexpr uint16_t ColorWarn = ST7735_YELLOW;
constexpr uint16_t ColorBad = ST7735_RED;
constexpr uint16_t ColorInfo = ST7735_CYAN;

void printProbeStatus(Adafruit_ST7735 &tft, ProbeStatus status) {
  if (status == ProbeStatus::Ok) {
    tft.setTextColor(ColorGood, ST7735_BLACK);
    tft.print(F("OK"));
  } else if (status == ProbeStatus::Saturating) {
    tft.setTextColor(ColorBad, ST7735_BLACK);
    tft.print(F("OVR"));
  } else {
    tft.setTextColor(ColorWarn, ST7735_BLACK);
    tft.print(F("ZERO"));
  }
}

uint16_t percentColor(int16_t percent) {
  if (percent > 80)
    return ColorBad;
  if (percent > 70)
    return ColorWarn;
  if (percent < 45)
    return ColorInfo;
  return ST7735_WHITE;
}

int16_t roundedTenth(float value) {
  const float adjusted =
      value >= 0.0f ? value * 10.0f + 0.5f : value * 10.0f - 0.5f;
  return static_cast<int16_t>(adjusted);
}

int16_t roundedWhole(float value) {
  const float adjusted = value >= 0.0f ? value + 0.5f : value - 0.5f;
  return static_cast<int16_t>(adjusted);
}

void printFixedTenth(Adafruit_ST7735 &tft, int16_t value) {
  int16_t displayValue = value;
  if (displayValue < 0) {
    tft.print('-');
    displayValue = -displayValue;
  }

  tft.print(displayValue / 10);
  tft.print('.');
  tft.print(displayValue % 10);
}

void printFixedHundredth(Adafruit_ST7735 &tft, int32_t value) {
  int32_t displayValue = value;
  if (displayValue < 0) {
    tft.print('-');
    displayValue = -displayValue;
  }

  tft.print(displayValue / 100);
  tft.print('.');
  const uint8_t fraction = static_cast<uint8_t>(displayValue % 100);
  if (fraction < 10) {
    tft.print('0');
  }
  tft.print(fraction);
}

int32_t adcCountToHundredthMillivolts(int16_t count) {
  return (static_cast<int32_t>(count) * 25L) / 32L;
}
} // namespace

DisplayManager::DisplayManager()
    : tft_(DeviceConfig::TftCsPin, DeviceConfig::TftDcPin,
           DeviceConfig::TftMosiPin, DeviceConfig::TftSclkPin,
           DeviceConfig::TftResetPin) {}

void DisplayManager::begin() {
  tft_.initR(INITR_18BLACKTAB);
  tft_.setRotation(3);
  tft_.fillScreen(ST7735_BLACK);
}

Adafruit_ST7735 &DisplayManager::rawTft() { return tft_; }

void DisplayManager::showStartup() {
  tft_.fillScreen(ST7735_BLACK);
  tft_.fillRect(0, 18, 160, 1, ColorInfo);
  tft_.fillRect(0, 110, 160, 1, ColorInfo);

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setTextSize(2);
  tft_.setCursor(38, 36);
  tft_.print(F("BiasPro"));

  tft_.setTextSize(1);
  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(74, 62);
  tft_.print(F("by"));
  tft_.setCursor(35, 78);
  tft_.print(F("ToneAlchemy.com"));
}

void DisplayManager::drawTubeSelection(const TubeProfile *profiles,
                                       uint8_t count, uint8_t selectedIndex) {
  tft_.fillScreen(ST7735_BLACK);

  tft_.setTextSize(1);
  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(6, 4);
  tft_.print(F("SELECT"));

  tft_.fillRect(8, 24, 144, 1, ColorPanel);
  tft_.fillRect(8, 42, 144, 1, ColorPanel);

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(16, 32);
  tft_.print(F("Target"));

  tft_.setCursor(10, 96);
  tft_.print(F("< > choose"));

  tft_.setCursor(94, 96);
  tft_.print(F("C start"));

  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(22, 116);
  tft_.print(F("Hold C tools"));

  updateTubeSelection(profiles, count, selectedIndex);
}

void DisplayManager::updateTubeSelection(const TubeProfile *profiles,
                                         uint8_t count, uint8_t selectedIndex) {
  tft_.fillRect(12, 48, 136, 28, ST7735_BLACK);

  tft_.setTextSize(2);
  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(18, 52);

  if (count > 0 && selectedIndex < count) {
    tft_.print(profiles[selectedIndex].label);
  } else {
    tft_.print(F("NONE"));
  }

  tft_.setTextSize(1);
  tft_.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft_.setCursor(96, 57);

  if (count > 0 && selectedIndex < count) {
    tft_.print(profiles[selectedIndex].maxDissipationWatts);
    tft_.print(F("W"));
  } else {
    tft_.print(F("--"));
  }
}

void DisplayManager::drawLiveBiasFrame(const TubeProfile &tube) {
  tft_.fillScreen(ST7735_BLACK);

  tft_.setTextSize(1);
  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(4, 3);
  tft_.print(F("LIVE BIAS"));

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(82, 3);
  tft_.print(tube.label);

  tft_.fillRect(3, 17, 74, 1, ColorPanel);
  tft_.fillRect(83, 17, 74, 1, ColorPanel);

  tft_.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft_.setCursor(30, 22);
  tft_.print(F("A"));
  tft_.setCursor(110, 22);
  tft_.print(F("B"));

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(8, 38);
  tft_.print(F("V"));
  tft_.setCursor(88, 38);
  tft_.print(F("V"));

  tft_.setCursor(8, 52);
  tft_.print(F("mA"));
  tft_.setCursor(88, 52);
  tft_.print(F("mA"));

  tft_.setCursor(8, 66);
  tft_.print(F("W"));
  tft_.setCursor(88, 66);
  tft_.print(F("W"));

  tft_.setCursor(8, 82);
  tft_.print(F("%"));
  tft_.setCursor(88, 82);
  tft_.print(F("%"));

  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(18, 111);
  tft_.print(F("MATCH"));

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(104, 116);
  tft_.print(F("C back"));

  liveValuesPrimed_ = false;
}

void DisplayManager::updateLiveBiasValues(const BiasReading &reading) {
  tft_.setTextSize(1);
  const int16_t percentA = roundedWhole(reading.percentA);
  const int16_t percentB = roundedWhole(reading.percentB);

  drawLiveTenth(roundedTenth(reading.voltsA), liveVoltsA10_, 24, 38,
                ST7735_WHITE);
  drawLiveTenth(roundedTenth(reading.voltsB), liveVoltsB10_, 104, 38,
                ST7735_WHITE);
  drawLiveTenth(roundedTenth(reading.milliampsA), liveMilliampsA10_, 24, 52,
                ST7735_WHITE);
  drawLiveTenth(roundedTenth(reading.milliampsB), liveMilliampsB10_, 104, 52,
                ST7735_WHITE);
  drawLiveTenth(roundedTenth(reading.wattsA), liveWattsA10_, 24, 66,
                ST7735_WHITE);
  drawLiveTenth(roundedTenth(reading.wattsB), liveWattsB10_, 104, 66,
                ST7735_WHITE);
  drawLiveWhole(percentA, livePercentA_, 24, 82, percentColor(percentA));
  drawLiveWhole(percentB, livePercentB_, 104, 82, percentColor(percentB));
  drawLiveTenth(roundedTenth(reading.currentDelta), liveDelta10_, 58, 111,
                ColorInfo);
  liveValuesPrimed_ = true;
}

void DisplayManager::drawLiveTenth(int16_t value, int16_t &previous, int16_t x,
                                   int16_t y, uint16_t color) {
  if (liveValuesPrimed_ && value == previous) {
    return;
  }

  previous = value;
  tft_.setTextColor(color, ST7735_BLACK);
  tft_.setCursor(x, y);
  printFixedTenth(tft_, value);
  tft_.print(F("  "));
}

void DisplayManager::drawLiveWhole(int16_t value, int16_t &previous, int16_t x,
                                   int16_t y, uint16_t color) {
  if (liveValuesPrimed_ && value == previous) {
    return;
  }

  previous = value;
  tft_.setTextColor(color, ST7735_BLACK);
  tft_.setCursor(x, y);
  tft_.print(value);
  tft_.print(F("  "));
}

void DisplayManager::drawSensorTelemetryFrame() {
  tft_.fillScreen(ST7735_BLACK);

  tft_.setTextSize(1);
  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(4, 3);
  tft_.print(F("SENSORS"));

  tft_.fillRect(0, 15, 160, 1, ColorPanel);

  tft_.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft_.setCursor(5, 22);
  tft_.print(F("A"));

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(14, 37);
  tft_.print(F("CATH"));

  tft_.setCursor(14, 51);
  tft_.print(F("PLATE"));

  tft_.fillRect(0, 67, 160, 1, ColorPanel);

  tft_.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft_.setCursor(5, 75);
  tft_.print(F("B"));

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(14, 90);
  tft_.print(F("CATH"));

  tft_.setCursor(14, 104);
  tft_.print(F("PLATE"));

  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(4, 119);
  tft_.print(F("RAW ADC"));
}

void DisplayManager::updateSensorTelemetryValues(
    const SensorTelemetryFrame &telemetry) {
  tft_.setTextSize(1);
  tft_.fillRect(46, 22, 112, 42, ST7735_BLACK);
  tft_.fillRect(46, 75, 112, 42, ST7735_BLACK);

  tft_.setCursor(134, 22);
  printProbeStatus(tft_, telemetry.probeA);

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(46, 37);
  tft_.print(telemetry.counts.cathodeA);
  tft_.setCursor(92, 37);
  printFixedHundredth(tft_,
                      adcCountToHundredthMillivolts(telemetry.counts.cathodeA));
  tft_.print(F("mV"));

  tft_.setCursor(46, 51);
  tft_.print(telemetry.counts.plateA);
  tft_.setCursor(92, 51);
  printFixedHundredth(tft_,
                      adcCountToHundredthMillivolts(telemetry.counts.plateA));
  tft_.print(F("mV"));

  tft_.setCursor(134, 75);
  printProbeStatus(tft_, telemetry.probeB);

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(46, 90);
  tft_.print(telemetry.counts.cathodeB);
  tft_.setCursor(92, 90);
  printFixedHundredth(tft_,
                      adcCountToHundredthMillivolts(telemetry.counts.cathodeB));
  tft_.print(F("mV"));

  tft_.setCursor(46, 104);
  tft_.print(telemetry.counts.plateB);
  tft_.setCursor(92, 104);
  printFixedHundredth(tft_,
                      adcCountToHundredthMillivolts(telemetry.counts.plateB));
  tft_.print(F("mV"));
}

void DisplayManager::drawProfileManager(const TubeProfile *profiles,
                                        uint8_t count, uint8_t selectedIndex) {
  tft_.fillScreen(ST7735_BLACK);

  tft_.setTextSize(1);
  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(5, 4);
  tft_.print(F("PROFILES"));

  tft_.fillRect(4, 20, 152, 1, ColorPanel);

  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(12, 31);
  tft_.print(F("Selected"));

  tft_.setCursor(7, 102);
  tft_.print(F("< > pick"));

  tft_.setCursor(76, 102);
  tft_.print(F("C edit"));

  tft_.setCursor(7, 116);
  tft_.print(F("Hold C cal"));

  updateProfileManagerSelection(profiles, count, selectedIndex);
}

void DisplayManager::updateProfileManagerSelection(const TubeProfile *profiles,
                                                   uint8_t count,
                                                   uint8_t selectedIndex) {
  tft_.fillRect(18, 48, 134, 28, ST7735_BLACK);

  tft_.setTextSize(2);
  tft_.setCursor(20, 50);

  if (count > 0 && selectedIndex < count) {
    tft_.print(profiles[selectedIndex].label);
  } else {
    tft_.print(F("EMPTY"));
  }

  tft_.setTextSize(1);
  tft_.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft_.setCursor(95, 55);

  if (count > 0 && selectedIndex < count) {
    tft_.print(profiles[selectedIndex].maxDissipationWatts);
    tft_.print(F("W "));
    tft_.print(profiles[selectedIndex].screenCurrentPermille);
    tft_.print(F("/1000"));
  } else {
    tft_.print(F("--"));
  }
}

void DisplayManager::drawCalibration(const CalibrationSettings &settings,
                                     uint8_t selectedField) {
  tft_.fillScreen(ST7735_BLACK);

  tft_.setTextSize(1);
  tft_.setTextColor(ColorInfo, ST7735_BLACK);
  tft_.setCursor(5, 4);
  tft_.print(F("CAL"));

  tft_.setCursor(6, 116);
  tft_.print(F("< > adjust  C next"));

  calibrationValuesPrimed_ = false;
  drawCalibrationRow(0, selectedField == 0, settings.voltageScaleA_centi);
  drawCalibrationRow(1, selectedField == 1, settings.voltageScaleB_centi);
  drawCalibrationRow(2, selectedField == 2, settings.shuntA_milliohm);
  drawCalibrationRow(3, selectedField == 3, settings.shuntB_milliohm);
  drawCalibrationRow(4, selectedField == 4, settings.voltageLimit);
  calibrationSelected_ = selectedField;
  calibrationValuesPrimed_ = true;
}

void DisplayManager::updateCalibrationValues(
    const CalibrationSettings &settings, uint8_t selectedField) {
  if (!calibrationValuesPrimed_) {
    drawCalibration(settings, selectedField);
    return;
  }

  if (selectedField != calibrationSelected_) {
    uint16_t oldValue = settings.voltageLimit;
    if (calibrationSelected_ == 0)
      oldValue = settings.voltageScaleA_centi;
    else if (calibrationSelected_ == 1)
      oldValue = settings.voltageScaleB_centi;
    else if (calibrationSelected_ == 2)
      oldValue = settings.shuntA_milliohm;
    else if (calibrationSelected_ == 3)
      oldValue = settings.shuntB_milliohm;
    drawCalibrationRow(calibrationSelected_, false, oldValue);
  }

  uint16_t value = settings.voltageLimit;
  if (selectedField == 0)
    value = settings.voltageScaleA_centi;
  else if (selectedField == 1)
    value = settings.voltageScaleB_centi;
  else if (selectedField == 2)
    value = settings.shuntA_milliohm;
  else if (selectedField == 3)
    value = settings.shuntB_milliohm;
  drawCalibrationRow(selectedField, true, value);

  calibrationSelected_ = selectedField;
  calibrationValuesPrimed_ = true;
}

void DisplayManager::drawCalibrationRow(uint8_t fieldIndex, bool selected,
                                        uint16_t value) {
  tft_.setTextSize(1);
  tft_.setTextColor(selected ? ST7735_YELLOW : ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(10, 24 + (fieldIndex * 17));
  tft_.print(selected ? '>' : ' ');

  if (fieldIndex == 0) {
    tft_.print(F("Scale A "));
    printFixedHundredth(tft_, value);
    tft_.print(F(" "));
  } else if (fieldIndex == 1) {
    tft_.print(F("Scale B "));
    printFixedHundredth(tft_, value);
    tft_.print(F(" "));
  } else if (fieldIndex == 2) {
    tft_.print(F("Shunt A "));
    tft_.print(value);
    tft_.print(F("mR "));
  } else if (fieldIndex == 3) {
    tft_.print(F("Shunt B "));
    tft_.print(value);
    tft_.print(F("mR "));
  } else {
    tft_.print(F("Limit "));
    tft_.print(value);
    tft_.print(F("V "));
  }
}

void DisplayManager::drawVoltageLockout(float voltsA, float voltsB,
                                        uint16_t limit) {
  tft_.fillScreen(ST7735_RED);

  tft_.setTextSize(2);
  tft_.setTextColor(ST7735_WHITE, ST7735_RED);
  tft_.setCursor(18, 14);
  tft_.print(F("VOLTAGE"));

  tft_.setCursor(18, 36);
  tft_.print(F("LOCKOUT"));

  tft_.setTextSize(1);
  tft_.setCursor(18, 68);
  tft_.print(F("A "));
  printFixedTenth(tft_, roundedTenth(voltsA));
  tft_.print(F("V"));

  tft_.setCursor(88, 68);
  tft_.print(F("B "));
  printFixedTenth(tft_, roundedTenth(voltsB));
  tft_.print(F("V"));

  tft_.setCursor(20, 90);
  tft_.print(F("Limit "));
  tft_.print(limit);
  tft_.print(F("V"));

  tft_.setCursor(16, 110);
  tft_.print(F("Wait safe band"));
}

void DisplayManager::drawHardwareFault() {
  tft_.fillScreen(ST7735_BLACK);

  tft_.setTextSize(2);
  tft_.setTextColor(ST7735_RED, ST7735_BLACK);
  tft_.setCursor(0, 30);
  tft_.print(F("HARDWARE FAULT"));

  tft_.setTextSize(1);
  tft_.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft_.setCursor(52, 70);
  tft_.print(F("CHECK I2C"));
}
