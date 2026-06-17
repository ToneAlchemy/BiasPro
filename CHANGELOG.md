- [Implemented] Added missing Startup Shortcut to jump straight into Calibration Mode when holding the Left Button during the splash screen.

- [Implemented] Removed fillRect from SensorTelemetry values and replaced with PROGMEM space padding to eliminate UI flicker.

- [Implemented] Removed Adafruit_ADS1X15 dependency and implemented direct manual I2C register override in HardwareIO.cpp to prevent EMI-induced freezing and save flash memory.
- [Fixed] Completely removed the hardware Watchdog Timer (WDT) due to a silicon bug in certain LGT8F328P Nano clones where the WDT prescaler maps `WDTO_4S` to 15ms and drops rapid `wdt_reset()` instructions, causing boot loop freezes in UI screens.
- [Implemented] Added `Wire.setWireTimeout(25000, true)` as a modern, superior software-level I2C freeze protection that avoids rebooting the microcontroller entirely when an EMP locks up the bus.
- [Implemented] Added "RAW SENSORS" (Diagnostic Mode) as an explicit virtual menu item at the end of the Tube Selection carousel, replacing the hidden Long-Center shortcut. Fixed the issue where Long-Center was required to exit Diagnostic mode by assigning the exit action to a standard short Center press, completely bypassing I2C micro-stutter timing issues.
- [Fixed] Shifted the yellow `30W 55/1000` text leftward on the Tube Selection screen to prevent the trailing zero from wrapping to the next line.
- [Fixed] Removed the yellow `DIAGNOSTIC` sub-text from the RAW SENSORS virtual profile to eliminate ugly text overlap.
- [Fixed] Added intelligent event caching to the `HardwareIO` layer, allowing it to poll for short button presses even while the processor is heavily blocked executing the 160ms raw ADC read loops. This completely fixes the unresponsive Short Center press exit bug in the Raw Sensors mode!
- [Fixed] Allowed `LongCenter` button press to exit the RAW SENSORS page in addition to the short `Center` press, ensuring users who hold the button can still return to the main menu.
