# BiasPro Public Changelog

All notable user-facing changes to the BiasPro firmware are documented in this file.

## [1.1.1] - 2026-07-11

### Added & Improved
- **Enhanced UI Layout:** Redesigned the "Tube Select" and "Profile Manager" screens to feature a vertically stacked and horizontally centered layout, making tube labels and specifications much easier to read.

- **Quick Escape Shortcut:** Added a new "Hold LEFT" hardware shortcut (with a visual on-screen footer) to quickly exit the Profile Manager and return directly to the main menu without needing to scroll.

- **Verified Stability:** Passed comprehensive mathematical and safety host-testing to guarantee the accuracy of bias calculations, EEPROM data integrity, and over-voltage lockouts.

## [1.1.0] - 2026-07-08

### Added
- **Live Calibration Voltage:** Added a real-time plate-voltage (Live V) readout on the Calibration screen. While adjusting the Scale or Shunt fields, users can now match the BiasPro directly against a digital multimeter (DMM) without leaving the screen.

## [1.0.0] - 2026-07-08

### Initial Release Highlights
First public release of the BiasPro firmware: a from-scratch, memory-safe, and highly robust rewrite for the dual-probe vacuum-tube bias meter. 

- **Dual-Probe Live Bias:** Real-time measurement of plate voltage, cathode current, and true plate dissipation (watts and percentage) with screen-current correction.
- **Tube Profile Manager:** Create, edit, and delete up to 10 tube profiles stored safely in EEPROM. Pre-loaded with 6 industry defaults (6L6GC, EL34, 6V6, EL84, KT88, 6550).
- **On-Board Calibration:** Calibrate voltage scales, shunt resistance, and over-voltage limits directly on the device—no source code edits required.
- **Advanced Safety Systems:** Features an active over-voltage lockout (with a 50V release hysteresis), boot-up hardware fault detection for the ADC/I2C bus, and software freeze protection to automatically recover from electrical interference.
- **Diagnostic Tools:** Includes a "RAW SENSORS" mode to view direct ADC counts and millivolts for advanced troubleshooting.
- **Data Protection:** Robust EEPROM storage with signature, range, and checksum validation that automatically restores safe defaults if data corruption is detected.
