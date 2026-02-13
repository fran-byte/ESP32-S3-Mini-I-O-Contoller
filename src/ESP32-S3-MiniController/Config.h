#pragma once
#include <Arduino.h>

// ===================== PIN CONFIG (ESP32-S3 SuperMini) =====================
// Pin definitions fully compatible with MagSenseUI hardware layout.

// ---------------------- Motor Driver Outputs ----------------------
// These pins typically drive optocouplers when controlling 24V logic systems.
#define PIN_CLOCK 1      // Step/clock output for motor speed control
#define PIN_DIR 2        // Direction control output
#define PIN_BRAKE 3      // Optional brake signal (depends on active profile)
#define PIN_STOP 4       // Optional stop/override signal
#define PIN_ENABLE 11    // Optional driver enable line

// ---------------------- Motor Driver Inputs -----------------------
// Inputs from driver for feedback and fault monitoring.
#define PIN_FG 12        // Frequency generator/tachometer input (interrupt-capable)
#define PIN_LD 13        // Alarm or fault input from motor driver

// ---------------------- OLED I2C Interface ------------------------
// Same pin assignment used by MagSenseUI for display compatibility.
#define PIN_OLED_SDA 9   // I2C data line for OLED
#define PIN_OLED_SCL 10  // I2C clock line for OLED

// ---------------------- Button Inputs -----------------------------
// Buttons are active LOW and follow the standard MagSenseUI arrangement.
#define PIN_BTN_UP     4 // Up navigation button
#define PIN_BTN_LEFT   5 // Left navigation button
#define PIN_BTN_RIGHT  6 // Right navigation button
#define PIN_BTN_DOWN   7 // Down navigation button

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
