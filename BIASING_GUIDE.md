# 🎛️ Tube Amplifier Biasing Guide
**Focus Bias Example: Fender Prosonic & Fixed-Bias Class AB Amps**

This guide outlines the professional procedure for biasing high-gain tube amplifiers using the **Bias Meter**. While written with the **Fender Prosonic** in mind, these principles apply to most fixed-bias amplifiers (e.g., Marshall, Peavey, Soldano).

> [!WARNING]
> **SAFETY FIRST:** Biasing requires the amplifier to be **powered on** and running with lethal high voltage present. If you are not comfortable working with live high voltage, take your amp to a qualified technician.

## 🛠️ Recommended Test Bench Setup
For the most accurate and safe results, arrange your equipment in this specific order:

1.  **Isolation Transformer:** (Wall Outlet → Transformer). Isolates the test gear from Earth Ground for safety.
2.  **Variac:** (Transformer → Variac). Allows you to set the input voltage exactly to your country's standard (e.g., 240V, 120V). This prevents wall voltage drift from skewing your bias readings.
3.  **Dim Bulb Limiter:** (Variac → Limiter → Amp).
    * **CRITICAL:** Set the limiter to **BYPASS** mode (or use only the built-in voltmeter/ammeter). Do *not* bias through the bulb, or voltage sag will give false readings.
4.  **Load Box / Speaker:** The amp **MUST** have a speaker load connected. A reactive load box is recommended for silence.

## 🎚️ Amp Settings (Before Powering On)
To ensure a stable "Idle" reading, the signal path must be silent.

* **Input:** **Unplug everything.** (Most Fender amps short the input to ground when unplugged, ensuring zero noise).
    * *Alternative:* If you must leave a cable in, connect it to a guitar with the volume turned to **0**. Never leave a cable dangling (it acts as an antenna for hum).
* **Volume / Gain:** Set all to **0**.
* **Master Volume:** Set to **0**.
* **EQ (Treble/Mid/Bass):** Setting does not matter (signal is effectively zero), but 12 o'clock is standard.
* **Reverb:** Set to **0**.

## ⚡ The "Prosonic Protocol" (Rectifier Switching)
The Fender Prosonic (and Mesa Rectifiers) allows you to switch between Solid State (SS) and Tube Rectification. This changes the Plate Voltage and Bias requirements significantly.

**Rule:** Always Calibrate and Bias in **Solid State Mode** (Position 3).

### Why?
1.  **Manufacturer Spec:** The Prosonic Service Manual explicitly states: *"Bias set to ... Rectifier switch position 3."*
2.  **Voltage Stability:** The SS Rectifier provides the stiffest, highest voltage (~465V). The Tube Rectifier sags significantly (~440V). Biasing at the highest voltage ensures safety across all modes.
3.  **Bias Shift:** When you switch from SS (Class AB) to Tube (Class A/AB), the bias current naturally rises by approx 5-10%. You must set the SS mode conservatively to prevent the Tube mode from running dangerously hot.

---

## 📉 The "Safe Maximum" Strategy (6L6GC)
Since the bias rises when switching to Tube Mode, you should **not** bias the SS mode to the maximum (70%).

**The Math (6L6GC Tubes - 30W Max Dissipation):**
* **Target:** We want the *hottest* mode (Tube Rectifier) to land at **70%** (Max Safe).
* **Adjustment:** Since Tube Mode runs ~5% hotter, we set the **SS Mode to 65%**.

| Mode | Target Dissipation % | Target Wattage (per tube) | Result |
| :--- | :--- | :--- | :--- |
| **Solid State (Set Here)** | **60% - 65%** | **18W - 19.5W** | Clean, tight, punchy. |
| **Tube Rectifier (Check Only)** | **65% - 70%** | **19.5W - 21W** | Warm, compressed, spongy. |

### Step-by-Step Procedure
1.  **Connect:** Install Bias Probes into the amp sockets and install tubes into the probes. Connect probes to the Bias Meter.
2.  **Power Up:** Turn on the amp (in **SS Mode**) and let tubes warm up for at least 5 minutes.
3.  **Calibrate:**
    * **Find a Safe Test Point:** Use the amp's schematic to find the safest B+ voltage point that powers the tubes (e.g., the Standby Switch or the main B+ fuse).
    * **WARNING:** Avoid probing Pin 3 directly at the tube socket. A slip of the probe can short Pin 3 to Pin 2 (Heater) or Pin 4 (Screen), causing catastrophic damage. Always use a safer upstream point for measurement.
    * Measure this voltage with a trusted DMM.
    * Enter **Calibration Mode** on the BiasPro (hold the Center button from the main screen to enter the Profiles screen, then hold Center again).
    * Adjust `Volt Scale` until the BiasPro's "Live V" readout matches your DMM (e.g., 465V).
4.  **Set Bias:**
    * Watch the **Watts** reading on the Bias Meter.
    * Adjust the bias pot on the amp until the meter reads approximately **19 Watts** (63%).
5.  **Verify:**
    * Flip the switch to **Tube Rectifier Mode** (Position 2).
    * Observe the readings. The Watts should rise slightly (e.g., to 21W).
    * As long as the hottest mode stays near or below **21W (70%)**, you are perfectly biased. 

---

## 📊 Reference: Dissipation Targets
Use these generic targets for Class AB fixed-bias amps if you are not using the "Safe Maximum" strategy above.

* **Cool (50-60%):** Longest tube life, maximum clean headroom, potentially sterile sound.
* **Warm (60-70%):** The "Golden Zone." Good balance of warmth, harmonic content, and tube life.

* **Hot (70%+):** Reduced tube life. High compression and earlier breakup. **Risk of Red Plating.**

---
## 📚 References
* **Fender Prosonic Service Manual (Rev E, 1996):**
    * *Schematic Reference:* 050173
    * *Part Number:* 021-1007-000 (Combo) / 021-2007-000 (Head)
    * *Relevant Section:* Page 7, Schematic Diagram (Note 3: "Bias set to -46 VDC...")



