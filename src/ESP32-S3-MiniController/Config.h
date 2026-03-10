#pragma once
#include <Arduino.h>

// ===================== PIN CONFIG (ESP32-S3 SuperMini) =====================
// Pin definitions based on hardware schematic.

// ---------------------- Motor Driver Outputs ----------------------
// These pins typically drive optocouplers when controlling 24V logic systems.
// Outputs: IO1, IO2, IO3, IO13
#define PIN_CLOCK 1      // Step/clock output for motor speed control
#define PIN_DIR 2        // Direction control output
#define PIN_BRAKE 3      // Optional brake signal (depends on active profile)
#define PIN_STOP 13      // Optional stop/override signal

// ---------------------- Motor Driver Inputs -----------------------
// Inputs from driver for feedback and fault monitoring.
// Inputs: IO8, IO11, IO12
#define PIN_FG 12        // Frequency generator/tachometer input (interrupt-capable)
#define PIN_LD 11        // Alarm or fault input from motor driver
#define PIN_ENABLE 8     // Optional driver enable input

// ---------------------- OLED I2C Interface ------------------------
// I2C pins for OLED display (SH1106 128x64).
#define PIN_OLED_SDA 9   // I2C data line for OLED
#define PIN_OLED_SCL 10  // I2C clock line for OLED

// ---------------------- Button Inputs -----------------------------
// Buttons are active LOW with internal pull-ups.
// UP/DOWN/LEFT/RIGHT: IO4/IO7/IO5/IO6
#define PIN_BTN_UP     4 // Up navigation button
#define PIN_BTN_DOWN   7 // Down navigation button
#define PIN_BTN_LEFT   5 // Left navigation button (back/cancel)
#define PIN_BTN_RIGHT  6 // Right navigation button (select/confirm)

// ---------------------- LEDC Clock Generator -----------------------
// Used to generate the CLOCK signal for the motor using PWM.
#define LEDC_CH_CLOCK 0     // LEDC channel used for the clock output
#define LEDC_TIMER_BITS 8   // PWM resolution (8‑bit timer)

// ---------------------- System Limits ------------------------------
#define MAX_PROFILES 8       // Maximum number of stored motor control profiles

// ---------------------- UI and Input Timing ------------------------
// Long‑press detection threshold for buttons.
#define LONG_PRESS_MS 600    // Duration (ms) to consider a SELECT long press

// RPM sampling window for tachometer processing.
#define RPM_SAMPLE_MS 1000   // Window (ms) for RPM measurement averaging

// ---------------------- Acceleration Ramp -------------------------
// Smooth ramp applied when starting or changing target speed.
#define RAMP_STEP_HZ    50       // Hz increment per ramp tick
#define RAMP_INTERVAL_MS 20      // Time (ms) between ramp ticks

// ---------------------- Start Timeout ---------------------------
// If no RPM is detected within this window after starting, motor is cut.
// Only active when the active profile has FG (tachometer) enabled.
#define START_TIMEOUT_MS 4000    // Max ms to wait for first RPM reading

// ---------------------- Admin Password --------------------------
// Password used to access admin-protected operations (delete admin motors, etc.)
// Stored once in NVS after first boot. Maximum 8 chars (A-Z, 0-9).
#define ADMIN_DEFAULT_PW "1234"  // Shown only if no password has been set yet
#define ADMIN_PW_MAX_LEN 8       // Max length of admin password

// ---------------------- Debug Flags -------------------------------
// Set any of these to 1 to enable verbose serial debug output.
#define DEBUG_BUTTONS 0
#define DEBUG_MOTOR 0
#define DEBUG_SPEED 1

// ---------------------- Language Selection ------------------------
// Supported UI languages.
enum Language
{
    LANG_EN = 0,    // English
    LANG_ES = 1     // Spanish
};
