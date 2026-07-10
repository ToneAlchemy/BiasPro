# Changelog

All notable changes to BiasPro are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Audited
- 2026-07-08: Audit v3 at v1.1.0 - overall **8.6/10** (up from 8.5 at v2). Firmware
  unchanged; all gates re-run green (compile 30,382 B / 98%, static check, shared-source
  integrity, physics 17/17). Scores rose on documentation/hygiene: the Calibration-screen
  lockout gap (v2 F-1) is now documented, the GFX pin matches the build (1.12.5), the audit
  template moved to v1.1 (Gate A retired), and the two-stage publish pipeline was verified.
  Report: `docs/Audit_Report/Audit_Report_v3_08-07-2026.md`.
- 2026-07-08: Audit v2 at v1.1.0 - overall **8.5/10** (up from 7.9 at v1). All six
  v1 warnings and the INFO backlog resolved; compile 30,382 B (98%) / 749 B RAM;
  static check, physics cross-check (17/17), and on-hardware boot all passed; host
  tests not executed (no toolchain). One new WARNING: the Calibration screen shows
  live plate voltage but does not evaluate the over-voltage lockout. Report:
  `docs/Audit_Report/Audit_Report_v2_08-07-2026.md`.

## [1.1.0] - 2026-07-08

### Added
- Live plate-voltage readout on the Calibration screen. While adjusting a Scale
  or Shunt field, the screen shows the live computed voltage for the active
  probe (probe A for Scale/Shunt A, probe B for Scale/Shunt B), refreshed on a
  ~350 ms non-blocking timer so it can be matched against a DMM. This is the
  "Live V" the Calibration Guide refers to. Flash 29,976 -> 30,382 B (98%,
  338 B headroom); RAM +3 B.

## [1.0.0] - 2026-07-08

First public release: a from-scratch, memory-safe rewrite of the original
BiasMeter firmware for the dual-probe vacuum-tube bias meter (Arduino Nano /
ATmega328P, ADS1115 16-bit ADC over I2C, ST7735 1.8" TFT). A full codebase
audit (7.9/10 - see `docs/Audit_Report/Audit_Report_v1_08-07-2026.md`) and its
remediation are folded into this release.

### Added
- Dual-probe live bias measurement: plate voltage, cathode current, and true
  plate dissipation (watts and %) with screen-current correction (entered in
  permille per tube profile).
- Tube Profile Manager: create/edit/delete up to 10 tube profiles in EEPROM,
  seeded with 6 defaults (6L6GC, EL34, 6V6, EL84, KT88, 6550).
- On-board calibration of voltage scale and shunt resistance per probe plus the
  over-voltage limit, stored in EEPROM - no source edits needed to calibrate.
- Over-voltage lockout with 50 V release hysteresis, plus a hardware-fault halt
  when the ADS1115 does not answer on the I2C bus at boot.
- "RAW SENSORS" diagnostic mode as a virtual item at the end of the tube-select
  carousel, showing raw ADC counts and millivolts.
- Startup shortcut: hold the Left button during the splash screen to jump
  straight into Calibration.
- Software I2C freeze protection via `Wire.setWireTimeout(25000, true)`, which
  recovers from a bus lockup without rebooting the microcontroller.
- Robust EEPROM storage: signature + range + checksum validation with automatic
  restore of safe defaults on corruption.
- Host-side C++17 calculation tests plus a source-level UI redraw regression
  check. The tests link the actual firmware sources (`MathCalculations`,
  `ProtectionSystem`, `StorageCore`), so they cannot drift from what ships.

### Changed
- Replaced the `Adafruit_ADS1X15` dependency with direct manual I2C register
  access to the ADS1115, for EMI resilience and flash savings.
- Calibration edits now persist to EEPROM on field-advance and on screen exit
  instead of on every Left/Right press, reducing wear on the checksum cell.
- Extracted the pure calculation/validation/protection logic into
  Arduino-independent units shared by the firmware and the host tests; the
  type/constant headers fall back to `<stdint.h>` when built off-target.
- Documentation corrected to match the code: the over-voltage monitor is scoped
  to Live Bias mode (not "continuous"), the calibration-guide exit steps, and
  the flash/RAM footprint figures.

### Fixed
- Sequenced the 16-bit ADS1115 byte assembly so MSB/LSB ordering is defined on
  any compiler, not just avr-gcc.
- A tube profile whose name is blanked to all spaces can no longer corrupt
  EEPROM and wipe the entire profile library back to defaults on the next boot.
- A failed I2C read is treated as an invalid frame: it can no longer release an
  active voltage lockout (a 0 V reading used to look "safe"), and Live Bias
  freezes the last good reading instead of flashing 0.0 V during a bus glitch.
- The voltage-lockout screen no longer flickers at ~4.5 Hz; it now uses an
  incremental redraw like the other screens.
- The boot hardware-fault title fits the 160 px row ("HW FAULT").
- Creating a profile at 10/10 capacity is refused up front instead of opening
  the editor and silently discarding the profile on save.
- RAW SENSORS short-Center exit reliability: button events are cached during the
  blocking ADC read loops, and a 250 ms Center debounce guard prevents bounce
  from instantly re-entering the screen; a Long-Center hold also exits.
- Tube-select yellow "30W 55/1000" text no longer wraps to the next line, and
  the overlapping "DIAGNOSTIC" sub-text was removed from the RAW SENSORS item.
- Telemetry values redraw without the `fillRect` flush that caused UI flicker.

### Removed
- The hardware Watchdog Timer (WDT). A silicon bug in some LGT8F328P Nano clones
  maps `WDTO_4S` to 15 ms and drops rapid `wdt_reset()` calls, causing boot-loop
  freezes; it is replaced by the `Wire` timeout above.

<!-- When the repository is published, add compare/tag links here, e.g.:
[1.0.0]: https://github.com/<owner>/<repo>/releases/tag/v1.0.0 -->

### Audited (2026-07-10)
- 2026-07-10: Audit v1 at v1.1.0 - overall **8.7/10** (up from 8.6 at v3). Firmware
  unchanged; all gates re-run green or documented as unavailable (no toolchain for compile/host-tests).
  Scores rose on documentation accuracy: README voltage divider physics corrected, calibration mode
  navigation and field ranges matched to code, and BIASING_GUIDE synced.
  Report: `docs/Audit_Report/Audit_Report_v1_10-07-2026.md`.

