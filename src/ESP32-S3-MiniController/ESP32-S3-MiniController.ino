#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include "Config.h"
#include "Strings_EN.h"
#include "Strings_ES.h"
#include "Buttons.h"
#include "Profiles.h"
#include "Motor.h"
#include "Ui.h"

// OLED instance (HW I2C with custom pins).
// SH1106 128x64, full buffer mode, hardware I2C using the configured SCL/SDA pins.
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_OLED_SCL, PIN_OLED_SDA);

// Core subsystems: input, storage, motor control, and user interface.
Buttons      buttons;
ProfileStore store;
MotorRuntime motor;
UI           ui;

void setup()
{
    // Serial console for diagnostics and optional debug traces.
    Serial.begin(115200);
    delay(1000); // Give time for USB CDC to enumerate and serial terminal to attach.

    Serial.println("\n\n=== MOTOR TESTER v2.0 ===");
    Serial.println("Build: " __DATE__ " " __TIME__);

    // ------------------- Debug configuration summary -------------------
#if DEBUG_BUTTONS
    Serial.println("DEBUG_BUTTONS: ENABLED");
#endif
#if DEBUG_MOTOR
    Serial.println("DEBUG_MOTOR: ENABLED");
#endif
#if DEBUG_SPEED
    Serial.println("DEBUG_SPEED: ENABLED");
#endif

    // ------------------- Pinout echo (useful for field checks) ---------
    Serial.println("\n--- Pin Configuration ---");
    Serial.print("CLOCK: ");  Serial.println(PIN_CLOCK);
    Serial.print("DIR: ");     Serial.println(PIN_DIR);
    Serial.print("BRAKE: ");   Serial.println(PIN_BRAKE);
    Serial.print("STOP: ");    Serial.println(PIN_STOP);
    Serial.print("ENABLE: ");  Serial.println(PIN_ENABLE);
    Serial.print("FG: ");      Serial.println(PIN_FG);
    Serial.print("LD: ");      Serial.println(PIN_LD);
    Serial.print("BTN_UP: ");  Serial.println(PIN_BTN_UP);
    Serial.print("BTN_DOWN: ");Serial.println(PIN_BTN_DOWN);
    Serial.print("BTN_SEL: "); Serial.println(PIN_BTN_SEL);

    // ------------------- I2C bus init for OLED -------------------------
    Serial.println("\n--- Initializing I2C ---");
    // Initialize Wire with custom SDA/SCL pins to match board routing.
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

    // ------------------- Buttons (debounced input) ---------------------
    Serial.println("--- Initializing Buttons ---");
    buttons.begin();

    // Print raw states to quickly verify wiring (remember: active-LOW).
    Serial.print("Button states - UP:");
    Serial.print(digitalRead(PIN_BTN_UP));
    Serial.print(" DOWN:");
    Serial.print(digitalRead(PIN_BTN_DOWN));
    Serial.print(" SEL:");
    Serial.println(digitalRead(PIN_BTN_SEL));

    // ------------------- Profile storage (NVS/Preferences) -------------
    Serial.println("--- Initializing Profile Store ---");
    store.begin();
    Serial.print("Profiles found: ");
    Serial.println(store.getCount());

    // ------------------- Motor subsystem -------------------------------
    Serial.println("--- Initializing Motor ---");
    motor.begin();

    // Load the active profile from non-volatile storage; fall back to defaults if none found.
    MotorProfile mp;
    if (!store.loadActive(mp))
    {
        mp.setDefaults();
        Serial.println("Using default profile");
    }
    else
    {
        Serial.print("Loaded profile: ");
        Serial.println(mp.name);
    }

    // Apply the selected profile (speed curve, limits, pins/flags, etc.)
    motor.applyProfile(mp);

    // ------------------- UI (display + input + model) ------------------
    Serial.println("--- Initializing UI ---");
    ui.begin(u8g2, buttons, store, motor);

    // Optional diagnostics at boot if UP+DOWN are held.
    // Useful to check sensors, I/O lines, and display without running the motor.
    ui.checkDiagAtBoot();

    // ------------------- User help -------------------------------------
    Serial.println("\n=== SETUP COMPLETE ===");
    Serial.println("Controls:");
    Serial.println("  UP/DOWN: Change speed");
    Serial.println("  SEL short: Menu");
    Serial.println("  SEL long: Start/Stop motor");
    Serial.println("========================\n");
}

void loop()
{
    // Poll inputs and generate one-shot events (edges and long-press).
    buttons.poll();

    // Periodically sample tachometer and update RPM (uses RPM_SAMPLE_MS window).
    motor.sampleRPM();

    // Drive the UI state machine: rendering, menu navigation, and actions.
    ui.loop();
}
