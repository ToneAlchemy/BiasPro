# 🎛️ Arduino Tube Amp BiasPro (Clean-Room Edition)

This repository contains the complete, mathematically hardened, clean-room firmware for a professional-grade, dual-probe vacuum tube bias meter. Designed for guitar amplifier technicians, this tool provides real-time, high-resolution measurements of Plate Voltage, Cathode Current, and True Plate Dissipation (Watts & %) for power tubes (e.g., 6L6GC, EL34, 6V6, KT88).

This firmware was completely re-architected from scratch using modern, memory-safe embedded C++ principles to eliminate dynamic memory leaks and ensure rock-solid physical safety lockouts.

---

## ⚠️ High-Voltage Safety Warning
**LETHAL VOLTAGES ARE PRESENT IN TUBE AMPLIFIERS.**
Tube amplifiers store lethal voltages in their filter capacitors even after being unplugged. This firmware is only a measurement tool; it **cannot** make unsafe wiring or handling safe. 
* Never connect the USB port to a computer while the probes are plugged into a live amplifier. 
* Always use a dummy load or speaker cabinet.
* If you are not trained in high-voltage safety, take your amplifier to a qualified technician. Build and use this project entirely at your own risk.

---

> [!CAUTION]
> ## ⚠️ WARNING: HIGH VOLTAGE SAFETY
>
> **Tube amplifiers contain lethal high voltage (often 400V–500V+), even when powered off and unplugged.**
>
> This project involves building and using test equipment that interfaces directly with these dangerous circuits.
> * **Risk of Death:** Incorrect wiring of the bias probes or misuse of this device can expose you to lethal current.
> * **Filter Capacitors:** High voltage can remain stored in the amplifier's capacitors long after the power is cut. You must know how to safely drain filter capacitors before opening any chassis.
> * **Qualified Personnel Only:** If you are not trained in high-voltage safety or are uncomfortable working with live circuits, **do not build or use this device**. Take your amplifier to a qualified technician instead.
>
> **DISCLAIMER & NO SUPPORT:** This project is provided "as is" with **no warranty and no technical support**. The author cannot provide assistance with assembly, troubleshooting, or code modifications. The author assumes no liability for injury, death, or equipment damage resulting from the use or construction of this device. **Use entirely at your own risk.**

> [!IMPORTANT]
> ### ⚡ Critical Safety: Capacitor Discharge
> Before touching any internal component or connecting probes to a chassis, you must verify the filter capacitors are drained.
>
> * **The Golden Rule:** Always measure with a multimeter to confirm 0VDC before touching anything.
> * **Discharge Tool:** Do not rely on the amp to drain itself. Use a dedicated **Capacitor Discharge Tool** to safely bleed off voltage.
>      * *DIY Tip:* You can build one using a high-wattage resistor (e.g., **2kΩ to 5kΩ, 5 Watts**) wired to a probe and an alligator clip for ground. This slows the discharge **and** prevents sparking, unlike a screwdriver short.
> * **One-Hand Rule:** When working on live amps, keep one hand in your pocket to prevent current from passing across your chest/heart.

> [!NOTE]
> ### 🛡️ Recommended Safety Gear
> If you are working inside a live amplifier chassis, we strongly recommend using the following safety equipment:
> 
> * **Isolation Transformer:** This isolates the amplifier from the mains earth ground, significantly reducing the risk of lethal shock if you accidentally touch a live component while grounded.
> * **Dim Bulb Limiter (Current Limiter):** Essential when powering up an amplifier for the first time after a repair. It prevents catastrophic damage (blown transformers) if there is a short circuit.
>      * **⚠️ CRITICAL WARNING:** Do **NOT** attempt to set the final bias while running through a Dim Bulb Limiter. The limiter drops the wall voltage, which lowers the plate voltage and gives false low bias readings. Always bypass the limiter for the final precision bias adjustment.
> * **Variac (Variable Transformer):** Highly recommended for two purposes:
>      1.  **Voltage Stabilization:** Wall voltage fluctuates. A Variac allows you to set the input voltage exactly to your country's standard (e.g., 240V in Australia, 120V in USA, 230V in UK). This ensures your bias readings are accurate and consistent, rather than drifting with high/low wall voltage.
>      2.  **Soft Start / Reforming:** Useful for slowly bringing up the voltage on vintage amplifiers to reform old capacitors safely.

---

## ⚙️ Hardware Specifications

* **MCU:** Arduino Nano / ATmega328P
* **Display:** ST7735 1.8" TFT SPI
* **ADC:** ADS1115 (16-bit) at I2C address `0x48`
* **Inputs:** Three tactile switches (Left, Right, Center)

### Immutable Pin Map
| Function | Arduino Nano Pin |
| :--- | :--- |
| **TFT SCLK** | D13 |
| **TFT MOSI** | D11 |
| **TFT CS** | D10 |
| **TFT DC** | D9 |
| **TFT RST** | D8 |
| **Button Center** | D7 (Uses `INPUT_PULLUP`) |
| **Button Right** | D6 (Uses `INPUT_PULLUP`) |
| **Button Left** | D5 (Uses `INPUT_PULLUP`) |
| **ADS1115 SDA** | A4 |
| **ADS1115 SCL** | A5 |

---

## 🛠️ Build & Flash Instructions

### 1. Pinned Dependencies
To guarantee a successful compile, you **must** use these exact library versions in the Arduino IDE:
* `Adafruit GFX Library` v1.12.6
* `Adafruit ST7735 and ST7789 Library` v1.11.0
* `Adafruit ADS1X15` v2.6.2
* `Adafruit BusIO` v1.17.4

### 2. CRITICAL: 99% Flash Memory Limit & The "Old Bootloader"
The firmware is mathematically hardened and highly optimised, compiling to exactly **30,702 bytes**. It will successfully flash to a standard Arduino Nano (Old Bootloader) with 56 bytes to spare. 

* **Using a Standard Nano or USB-C Clone:** The firmware *will* successfully flash to a standard Arduino Nano using the `Processor: ATmega328P (Old Bootloader)` setting in the Arduino IDE. The old bootloader leaves exactly 30,720 bytes of usable flash space, meaning this firmware fits with roughly **18 bytes to spare**. It works perfectly, but you cannot add any additional features or text without overflowing the memory.

* **If your upload fails (or you wish to add features):** You will need to upgrade your Nano to the modern *Optiboot* bootloader. To do this, burn the Optiboot bootloader to your Nano using an ISP programmer, and change your board selection in the Arduino IDE from "Arduino Nano" to **"Arduino Uno"**. Optiboot only consumes 0.5KB, which will instantly free up an additional 1.5KB of flash space.


---

## 🕹️ User Guide: Navigation & Firmware Quirks

Because this firmware operates with only **18 bytes** of free flash memory remaining, several UI decisions were made to prioritise mathematical safety over visual flair. 

### Basic Navigation
* **Left / Right Buttons:** Scroll through menus or adjust values.
* **Center Button (Short Click):** Select an item, confirm an edit, or advance to the next field.
* **Center Button (Long Hold):** Open the Tool Menu, access Sensor Telemetry, or return to the main screen.

### Firmware Quirks (By Design)

**1. The "Invisible Spaces" in the Profile Editor**
When creating a new tube profile, the Name field holds exactly **6 characters**. If you type a short name like `6V6` (3 characters), you still have 3 invisible blank spaces remaining. 
* **The Quirk:** The system will not drop down to the "Max Watts" line until you clear all 6 slots.
* **The Solution:** Simply rapid-click the **Center** button 3 times to skip past the invisible empty spaces and move to the next line.

**2. Screen Grid Current (`Src`) is in Permille (/1000)**
To calculate True Plate Dissipation, the BiasPro subtracts the Screen Grid Current from the Cathode reading. Floating-point decimals (like 5.5%) crash the Arduino's memory limit. Therefore, the firmware uses **Permille (parts-per-thousand)**. 
* To enter a percentage, simply multiply it by 10.
* **5.5%** = enter `55` /1000
* **13.0%** = enter `130` /1000
* **4.0%** = enter `40` /1000

---

## 🧪 Safe Biasing Protocol

1. **Bench Setup:** Connect your amplifier to a speaker load or reactive load box. An Isolation Transformer and a Variac are highly recommended for stable wall-voltage readings.
2. **Probe Connection:** Ensure the amplifier is powered OFF. Remove the power tubes, insert the BiasPro probes into the amp sockets, and place the tubes into the top of the probes.
3. **Select Tube Profile:** Power on the BiasPro. Select the matching tube profile (e.g., `6L6GC`). Press Center to enter **Live Bias Mode**.
4. **Power the Amp:** Turn the amplifier ON. Wait for the tubes to warm up, then disengage standby. 
5. **Adjust Bias:** Watch the **%** and **Watts** readings. Adjust your amplifier's internal bias potentiometer until the hottest tube reaches your desired target:
    * **Cool (50% - 60%):** Maximum clean headroom, longest tube life.
    * **Warm (60% - 70%):** The "Golden Zone". Balanced harmonics and warmth.
    * **Hot (70%+):** **DANGER.** High compression, short tube life, risk of red-plating.

### Hardware Faults & Lockouts

* **HARDWARE FAULT: CHECK I2C:** The ADS1115 ADC has disconnected. The meter will permanently halt to prevent phantom voltage readings. Power cycle to reset.

* **VOLTAGE LOCKOUT:** If the probes detect a voltage spike above your configured safety limit (default 600V), the screen turns red and locks. You must wait for the voltage to drop at least 50V below the limit before the meter releases the lock. Do not attempt to bypass this.

---

## ⚖️ Clean-Room Provenance & License
This repository is a Clean-Room rewrite. It implements a strict, behaviour-only specification based on the physical requirements of a high-voltage bias meter. It does not contain any legacy source code, structural organisation, or mechanical translation from previous GPL/CC-licensed Arduino sketches.

This project is released under the **MIT License**. See `LICENSE` for details.