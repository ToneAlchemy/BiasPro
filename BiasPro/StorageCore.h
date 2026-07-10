#pragma once

#include "DomainTypes.h"

// Arduino-independent storage logic shared by the firmware's NonVolatileStorage
// layer (which adds the EEPROM I/O) and the host calculation tests. Keeping the
// checksum, validation, default, clamp, and recovery logic here means the tests
// exercise the real firmware code instead of a hand-maintained transcription
// that could silently drift from it.

uint8_t checksumCalibration(const CalibrationSettings& settings);
bool calibrationIsValid(const CalibrationSettings& settings);

// Force `settings` into the valid ranges, stamp the signature, and recompute the
// checksum. Used by saveCalibration and mirrored by the boundary tests.
void clampCalibration(CalibrationSettings& settings);
CalibrationSettings defaultCalibration();

uint8_t checksumProfiles(const TubeProfile* profiles, uint8_t count);
bool profileLooksValid(const TubeProfile& profile);
void copyLabel(char* target, const char* source);
uint8_t defaultProfiles(TubeProfile* profiles, uint8_t capacity);

// Decide how many stored profiles to trust. `profiles` must already be filled
// from storage for indices [0, storedCount). Returns the count to use; when the
// data is missing or corrupt it overwrites `profiles` with the built-in defaults
// and (if provided) sets *usedDefaults to true so the caller can persist them.
uint8_t recoverProfiles(TubeProfile* profiles, uint8_t storedCount,
                        uint8_t storedChecksum, uint8_t capacity,
                        bool* usedDefaults = nullptr);
