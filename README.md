
***

# ESP32-S3-MiniController (v1.0)

A compact **motor tester / I/O controller** based on the **ESP32‑S3 SuperMini**, with a **1.3" SH1106 OLED**, three **cursor‑style buttons** (UP/DOWN/SEL), and **configurable opto‑isolated I/O**.  
It provides a clean **on‑device UI** for setting speed, direction, brake, enable/stop, and for managing **stored motor profiles** with persistent settings (NVS). Supports **English/Spanish**, **serial telemetry**, and a **diagnostics** mode.

> **Key idea:** With optocouplers on the I/O lines, you can safely interface **24 V logic** drivers while keeping the ESP32 at **3.3 V**.  
> **Important:** If you **swap the optocouplers’ orientation**, the **signal direction inverts**—the same ESP32 pin can function as an **output** (driving the opto LED) or an **input** (reading the opto transistor). The firmware lets you **configure that pin’s role** accordingly.

***

## ✨ Features

*   **OLED UI** (SH1106 128×64) with **UP/DOWN** for speed changes, **SEL short** for menu, **SEL long** for Start/Stop.
*   **Configurable profiles** (name, brake/enable/stop presence, LD/FG support & polarity, PPR, max clock Hz), stored in **NVS**.
*   **LEDC clock generation** at **50% duty** with dynamic frequency changes (0…maxClockHz).
*   **RPM measurement** from FG pulses (PPR) with 1 s sampling and **FG‑loss safety** (auto reduce to ¼ speed).
*   **English / Español** language switching (persisted).
*   **Serial telemetry** (optional, persisted).
*   **Diagnostics** screen; **boot‑diagnostics** if **UP+DOWN** are held at power‑on.
*   Clean **debounce & edge detection** for buttons with **long‑press** on SELECT.

***

## 🛠️ Hardware Overview

**Target board:** ESP32‑S3 SuperMini (ESP32‑S3).  
**Display:** 1.3" SH1106 128×64, I²C.  
**Buttons:** UP, DOWN, SEL (active‑LOW).  
**I/O to driver:** CLOCK (PWM), DIR, optional BRAKE/STOP/ENABLE.  
**Inputs from driver:** FG (tach), LD (alarm/fault).

**Default pin map (see `Config.h`):**

```cpp
// Motor driver outputs (via optos if 24V logic)
#define PIN_CLOCK   1
#define PIN_DIR     2
#define PIN_BRAKE   3     // optional per profile
#define PIN_STOP    4     // optional per profile
#define PIN_ENABLE  11    // optional per profile

// Inputs from motor driver
#define PIN_FG      12    // tachometer (interrupt)
#define PIN_LD      13    // alarm input

// OLED I2C (MagSenseUI compatible)
#define PIN_OLED_SDA 9
#define PIN_OLED_SCL 10

// Buttons (active LOW)
#define PIN_BTN_UP    5
#define PIN_BTN_DOWN  6
#define PIN_BTN_SEL   7

// LEDC clock (PWM)
#define LEDC_CH_CLOCK   0
#define LEDC_TIMER_BITS 8
```

### 🔌 About the Optocouplers (Important)

*   Each motor‑interface signal can be **opto‑isolated**.
*   **If you swap the optocoupler orientation**, the **direction reverses**:
    *   **Output mode:** the ESP32 drives the **opto LED** → the **transistor** pulls the remote side (use proper series resistors and respect current limits).
    *   **Input mode:** the remote side drives the **opto LED** → the **transistor** pulls the ESP32 side (use pull‑ups; inputs are **active‑LOW** in this project).
*   Because of this, **the same ESP32 pin can be configured as input or output** depending on how the optocoupler is installed. The firmware (profiles + pin modes) supports both roles—**just wire and configure consistently**.

> **Safety:** When interfacing 24 V logic or inductive loads, use appropriate resistors, diodes, and isolation distances. Validate with a current‑limited bench supply before connecting a live driver.

***

## 🧩 Firmware Architecture

**Toolchain:** Arduino framework on ESP32‑S3.

**Main modules:**

*   `Config.h` – Pins, constants (I²C pins, debounce times, LEDC bits, RPM sample period, debug flags, language enum).
*   `Buttons.h` – Poll‑based debounce (50 ms), falling‑edge events, **one‑shot** getters (`upPressed()`, `downPressed()`, `selPressed()`), and **long‑press** (`selLong()`).
*   `Profiles.h` – `MotorProfile` (name, hasBrake/FG/LD/Stop/Enable, polarities, PPR, maxClockHz) + `ProfileStore` (NVS persistence under `"motors"` namespace with `count` and `active` indices).
*   `Motor.h` – `MotorRuntime`: LEDC clock control, direction/brake/enable/stop outputs with profile‑driven polarities, FG **ISR** counting, RPM compute & **FG‑loss safety**, telemetry, language persistence (`"sys"` namespace).
*   `Strings_EN.h`, `Strings_ES.h` – Localized UI string tables (`struct Strings`).
*   `Ui.h` – State‑machine UI for HOME, MENU, SELECT\_MOTOR, ADD‑WIZARD, SETTINGS (Language/Telemetry), ABOUT, DIAGNOSTICS.
*   `main.cpp` – Initializes Serial, Wire, buttons, profile store, motor, UI; loads active profile (or defaults), applies it, checks boot‑diagnostics, and runs the main loop.

***

## 🖥️ UI & Controls

*   **HOME**
    *   Shows **RPM** (if FG present), **Speed (Hz)**, and status lines (**DIR**, optional **BRAKE/ENABLE/LD**).
    *   **UP/DOWN:** change target speed (coarse steps).
    *   **SEL short:** open **MENU**.
    *   **SEL long:** **Start/Stop** the motor.

*   **MENU** (dynamic)
    *   **Start/Stop**, **Set DIR = CW/CCW**, **Brake ON/OFF** (if present),
    *   **Select Motor**, **Add Motor**, **Delete Active** (if any),
    *   **Settings**, **About**, **Back**.
    *   **Note:** Long‑press is **disabled** inside menus.

*   **Add Motor Wizard**
    *   Steps: **Name → Has BRAKE → Has FG → Has LD → LD polarity → Has STOP → STOP polarity → Has ENABLE → ENABLE polarity → PPR → Max CLOCK Hz → Save?**
    *   Name editor: rotate characters with UP/DOWN; **END** marker finalizes.
    *   On **Save=YES**, profile is stored and made **active**.

*   **Settings**
    *   **Language:** English / Español (persisted).
    *   **Telemetry:** **ON/OFF** (persisted).
    *   **Back**.

*   **About**: author, version, build date.

*   **Diagnostics**: live button levels, LD status, RPM, clock Hz, direction.
    *   **Boot shortcut:** hold **UP+DOWN** at power‑on.
    *   **Exit:** **long‑press SELECT**.

***

## ⚙️ Motor Control Details

*   **Clock generation:** ESP32 **LEDC** on channel **0**, **8‑bit** resolution, **50% duty**. Frequency is re‑attached on the fly (`ledcDetach/Attach`) to minimize artifacts when changing `Hz`.
*   **Direction/Brake/Enable/Stop:**
    *   Pins are updated by `applyOutputs()` honoring each profile’s **presence** and **active polarity** flags.
    *   **Stop line** is asserted when **not running** (polarity per profile).
*   **RPM sampling:**
    *   FG ISR counts **pulses**, sampled every `RPM_SAMPLE_MS` (default **1000 ms**).
    *   `rpm = (pulses * 60) / PPR`.
    *   **FG‑loss safety:** if **running** and **clock>0** but **rpm==0**, automatically reduce `targetHz` to **¼ of current** to mitigate stalls or feedback loss.

***

## 📦 Profiles & Persistence (NVS)

*   **Profile fields:**  
    `name`, `hasBrake`, `hasFG`, `hasLD`, `ldActiveLow`, `hasStop`, `stopActiveHigh`, `hasEnable`, `enableActiveHigh`, `ppr`, `maxClockHz`.
*   **Storage:**
    *   Namespace: `"motors"`. Keys: `"count"`, `"active"`, and per‑profile `"m{idx}_..."` keys for all fields.
    *   `append()` grows `count`. `remove(idx)` compacts entries and clears the last slot. If `active` goes out of range, it falls back to first (or none).
*   **System settings:**
    *   Namespace: `"sys"`. Keys: `"tele"` (bool), `"lang"` (uchar).

***

## ⌨️ Buttons & Debounce

*   Inputs are **`INPUT_PULLUP`** and **active‑LOW**.
*   **50 ms debounce**; **falling‑edge** generates one‑shot events.
*   **`selLong()`** fires once when hold time exceeds `LONG_PRESS_MS` (default **600 ms**).

***

## 🧪 Telemetry

When enabled (Settings → Telemetry), the firmware periodically prints a one‑line snapshot:

    RPM:<rpm> Hz:<currentHz> Target:<targetHz> DIR:<CW|CCW> LD:<ALARM|OK>

Baud rate: **115200**.

***

## 🔧 Build & Flash

*   **Requirements**
    *   **Arduino IDE** (or **PlatformIO**) with **ESP32 Arduino** core (ESP32‑S3).
    *   **Libraries:**
        *   **U8g2** by olikraus (for SH1106).
        *   **Preferences** (bundled with ESP32 core).
*   **Board setup**
    *   Select an **ESP32‑S3** target (e.g., ESP32S3 Dev Module / SuperMini variant).
    *   3.3 V I/O. USB Serial at **115200**.
*   **Wiring**
    *   Connect **OLED I²C** to **SDA=GPIO9**, **SCL=GPIO10** (as in `Config.h`).
    *   Buttons to **GPIO5/6/7** with pull‑ups (already internal).
    *   Motor I/O to GPIOs per pin map.
    *   If using **opto‑isolation**, wire orientation according to desired **direction** (see optocoupler note above).
*   **Compile & Flash**
    *   Open the project, verify, and upload.
    *   Open Serial Monitor (115200) to see boot logs and optional telemetry.

***

## 🔌 Wiring Examples (Optocouplers)

> The following are **generic** examples—always check your optocoupler’s datasheet (IF, CTR, VCE(sat), isolation voltage), series resistors, and your driver’s I/O specs.

*   **ESP32 → Driver (OUTPUT via opto)**
    *   ESP32 GPIO ── R(series) ──► Opto **LED+**; **LED‑** → GND.
    *   Opto **Transistor** on driver side pulls the driver input line (use pull‑up or pull‑down as required by the driver).
    *   Configure the firmware pin as **OUTPUT** and set the appropriate **active polarity** in the profile.

*   **Driver → ESP32 (INPUT via opto)**
    *   Driver output ── R(series) ──► Opto **LED+**; **LED‑** → driver GND.
    *   Opto **Transistor** side pulls the ESP32 GPIO to GND (use ESP32 **`INPUT_PULLUP`**).
    *   Configure the firmware pin as **INPUT**; in this project **inputs are active‑LOW** by default (e.g., `ldActiveLow = true`).

*   **Swapping orientation** flips the direction; the **same ESP32 pin** can thus be **repurposed** by changing how the optocoupler is installed and updating the profile/pin mode.

***

## 📁 Repository Layout

    /src
      main.cpp           // Setup, initialization, system loop
      Config.h           // Pins, constants, debug flags
      Buttons.h          // Debounced button handling (edges + long-press)
      Profiles.h         // MotorProfile + ProfileStore (NVS)
      Motor.h            // MotorRuntime: LEDC, RPM, FG ISR, outputs, telemetry/lang
      Ui.h               // UI state machine (home/menu/wizard/settings/about/diag)
      Strings_EN.h       // English strings
      Strings_ES.h       // Spanish strings

***

## 🧯 Safety Notes

*   Working with **24 V logic** and motor drivers can be hazardous.
*   Verify **isolation** and **grounds**, use current‑limited supplies for first power‑up, and maintain safe distances/creepage.
*   Always test new profiles at **low speeds** and without load before connecting a real motor.
*   Ensure you have an **independent emergency stop**.

***

## 📜 License

Add your preferred license here (e.g., MIT, Apache‑2.0).  
Include a `LICENSE` file in the repository.

***

## 🙌 Credits

*   **Author:** Fran‑Byte
*   OLED rendering: **U8g2** by olikraus
*   Built with the **ESP32‑S3 Arduino** core

***

## 🗺️ Roadmap / Ideas

*   Soft‑ramp (accel/decel) for target Hz
*   More profile slots or export/import over Serial
*   Additional languages / font packs
*   Optional CRC/versioning for NVS profile schema

***

### Quick Start

1.  Wire OLED to **SDA=9, SCL=10**, buttons to **GPIO5/6/7**, and your motor driver via optos to the defined pins.
2.  Build & flash; open Serial Monitor at **115200**.
3.  On first boot, **default profile** loads.
4.  Use **UP/DOWN** to change speed; **SEL long** to **Start/Stop**.
5.  Open **Menu → Add Motor** to create your first profile.
6.  If you reversed an optocoupler’s orientation, **update the profile** and **pin mode** so the firmware treats that line as **input or output** accordingly.

***


