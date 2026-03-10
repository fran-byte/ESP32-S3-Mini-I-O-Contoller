#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "Profiles.h"

// Simple, header-only max helper to avoid <algorithm> on embedded targets.
template <typename T>
T simple_max(T a, T b) { return (a > b) ? a : b; }

class MotorRuntime
{
public:
    void begin()
    {
        // ---------------- GPIO directions ----------------
        pinMode(PIN_CLOCK,  OUTPUT);     // PWM/clock output for motor
        pinMode(PIN_DIR,    OUTPUT);     // Direction output
        pinMode(PIN_BRAKE,  OUTPUT);     // Optional brake line
        pinMode(PIN_STOP,   OUTPUT);     // Optional stop line

        pinMode(PIN_ENABLE, INPUT_PULLUP); // Optional enable input (changed from output)
        pinMode(PIN_FG,     INPUT_PULLUP); // Tachometer input (FG), active edge = RISING
        pinMode(PIN_LD,     INPUT_PULLUP); // Fault/alarm input (LD), polarity set by profile

        // ---------------- LEDC clock setup ----------------
        // Attach LEDC (ESP32 PWM) to PIN_CLOCK with an initial frequency and resolution.
        // We start at 1 kHz and 8-bit resolution, but reattach dynamically in setClock().
        ledcAttach(PIN_CLOCK, 1000, LEDC_TIMER_BITS);
        ledcWrite(PIN_CLOCK, 0);         // Duty 0% (motor stopped)

        // ---------------- Tachometer ISR ------------------
        // Count FG pulses on rising edge to compute RPM periodically.
        attachInterrupt(digitalPinToInterrupt(PIN_FG), isrFG, RISING);

        // ---------------- System settings (NVS) -----------
        // Load persisted telemetry and language preferences.
        sysPrefs.begin("sys", false);
        telemetryOn = sysPrefs.getBool("tele", false);
        lang = (Language)sysPrefs.getUChar("lang", (uint8_t)LANG_ES);
        sysPrefs.end();

#if DEBUG_MOTOR
        Serial.println("Motor initialized");
        Serial.print("Telemetry: ");
        Serial.println(telemetryOn ? "ON" : "OFF");
#endif
    }

    // Apply a new motor profile (I/O capabilities, limits, polarities, etc.)
    // Resets runtime flags and targets to safe defaults.
    void applyProfile(const MotorProfile &p)
    {
        prof     = p;
        dirCW    = true;
        brakeOn  = false;
        enabled  = true;
        running  = false;
        targetHz = 1000;       // Default target clock (Hz)
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Profile applied: ");
        Serial.println(prof.name);
#endif
    }

    // Push current runtime control state to hardware pins, honoring profile options:
    //  - Direction line always active
    //  - Brake/Stop only if present in profile, with correct active polarity
    //  - Enable is now an INPUT, so we read it instead of writing to it
    void applyOutputs()
    {
        digitalWrite(PIN_DIR, dirCW ? HIGH : LOW);

        if (prof.hasBrake)
            digitalWrite(PIN_BRAKE, brakeOn ? HIGH : LOW);

        if (prof.hasStop)
        {
            // When not running, assert STOP according to profile polarity.
            bool active = !running;
            bool level = prof.stopActiveHigh ? active : !active;
            digitalWrite(PIN_STOP, level ? HIGH : LOW);
        }

        // Note: ENABLE is now an input pin, so we don't control it
        // The 'enabled' flag reflects the state we READ from PIN_ENABLE
        if (prof.hasEnable)
        {
            int enableLevel = digitalRead(PIN_ENABLE);
            enabled = prof.enableActiveHigh ? (enableLevel == HIGH) : (enableLevel == LOW);
        }
    }

    // Start the motor using ramp acceleration from 0 to targetHz.
    void start()
    {
        running = true;
        startTimeoutFired  = false;

        // Begin ramp from zero
        rampCurrentHz      = 0;
        rampActive         = true;
        lastRampTick       = millis();

        // Arm start timeout (only meaningful when profile has FG)
        if (prof.hasFG)
        {
            startTimeoutActive = true;
            startTimeoutStart  = millis();
        }

        // Apply clock at 0 so LEDC is attached; ramp will raise it
        setClock(0);
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Motor STARTED (ramping to ");
        Serial.print(targetHz);
        Serial.println(" Hz)");
#endif
    }

    // Stop the motor with a deceleration ramp, then cut clock.
    void stop()
    {
        // Cancel start-timeout monitoring
        startTimeoutActive = false;

        // If currently ramping up, stop the ramp immediately
        rampActive    = false;
        rampCurrentHz = 0;

        running = false;
        setClock(0);
        applyOutputs();

#if DEBUG_MOTOR
        Serial.println("Motor STOPPED");
#endif
    }

    // Configure the LEDC clock frequency and duty cycle.
    // Re-attaches LEDC with the requested frequency to minimize jitter.
    void setClock(uint32_t hz)
    {
        if (hz == 0)
        {
            // Duty 0% to ensure no pulses; keep last configured frequency irrelevant.
            ledcWrite(PIN_CLOCK, 0);
            currentHz = 0;
            return;
        }

        // Enforce profile limit.
        if (hz > prof.maxClockHz)
            hz = prof.maxClockHz;

        // Reconfigure LEDC to the new frequency with the chosen resolution.
        // Note: ledcDetach/Attach pattern avoids artifacts when changing freq.
        ledcDetach(PIN_CLOCK);
        ledcAttach(PIN_CLOCK, hz, LEDC_TIMER_BITS);
        ledcWrite(PIN_CLOCK, 128); // ~50% duty with 8-bit resolution
        currentHz = hz;

#if DEBUG_MOTOR
        Serial.print("Clock set to ");
        Serial.print(hz);
        Serial.println(" Hz");
#endif
    }

    // Coarse speed increase with tiered step sizes for fast navigation:
    //  0   ->  100 Hz
    // <1k -> +100 Hz
    // <5k -> +500 Hz
    // else -> +1000 Hz
    // Clamped to profile max, and applied immediately if running.
    void stepSpeedUp()
    {
        uint32_t oldTarget = targetHz;

        if (targetHz < prof.maxClockHz)
        {
            if (targetHz == 0)
            {
                targetHz = 100;
            }
            else if (targetHz < 1000)
            {
                targetHz += 100;
            }
            else if (targetHz < 5000)
            {
                targetHz += 500;
            }
            else
            {
                targetHz += 1000;
            }
        }

        if (targetHz > prof.maxClockHz)
            targetHz = prof.maxClockHz;

#if DEBUG_SPEED
        Serial.print("Speed UP: ");
        Serial.print(oldTarget);
        Serial.print(" -> ");
        Serial.print(targetHz);
        Serial.print(" Hz (running: ");
        Serial.print(running ? "YES" : "NO");
        Serial.println(")");
#endif

        if (running)
        {
            // Re-arm ramp toward new target (smooth speed change)
            rampActive   = true;
            lastRampTick = millis();
        }
    }

    // Coarse speed decrease mirroring the tiered strategy above:
    // >5k -> -1000 Hz
    // >1k -> -500 Hz
    // >100 -> -100 Hz
    //  >0  ->  0 Hz (stop target)
    // Applied immediately if running.
    void stepSpeedDown()
    {
        uint32_t oldTarget = targetHz;

        if (targetHz > 5000)
        {
            targetHz -= 1000;
        }
        else if (targetHz > 1000)
        {
            targetHz -= 500;
        }
        else if (targetHz > 100)
        {
            targetHz -= 100;
        }
        else if (targetHz > 0)
        {
            targetHz = 0;
        }

#if DEBUG_SPEED
        Serial.print("Speed DOWN: ");
        Serial.print(oldTarget);
        Serial.print(" -> ");
        Serial.print(targetHz);
        Serial.print(" Hz (running: ");
        Serial.print(running ? "YES" : "NO");
        Serial.println(")");
#endif

        if (running)
        {
            // Re-arm ramp toward new (lower) target
            rampActive   = true;
            lastRampTick = millis();
        }
    }

    // Set absolute direction (CW = true, CCW = false) and push to hardware.
    void setDirCW(bool cw)
    {
        dirCW = cw;
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Direction set to ");
        Serial.println(cw ? "CW" : "CCW");
#endif
    }

    // Toggle direction and push to hardware.
    void toggleDir()
    {
        dirCW = !dirCW;
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Direction toggled to ");
        Serial.println(dirCW ? "CW" : "CCW");
#endif
    }

    // Toggle brake if available in the profile.
    void toggleBrake()
    {
        if (prof.hasBrake)
        {
            brakeOn = !brakeOn;
            applyOutputs();

#if DEBUG_MOTOR
            Serial.print("Brake toggled to ");
            Serial.println(brakeOn ? "ON" : "OFF");
#endif
        }
    }

    // Read ENABLE input status if available in the profile.
    // Note: ENABLE is now an input pin, so we read it instead of toggling
    bool isEnabled() const
    {
        if (!prof.hasEnable)
            return true; // Assume enabled if not present

        int enableLevel = digitalRead(PIN_ENABLE);
        return prof.enableActiveHigh ? (enableLevel == HIGH) : (enableLevel == LOW);
    }

    // Read LD (fault/alarm) input, honoring profile polarity.
    bool ldAlarm() const
    {
        if (!prof.hasLD)
            return false;

        int v = digitalRead(PIN_LD);
        bool alarm = prof.ldActiveLow ? (v == LOW) : (v == HIGH);
        return alarm;
    }

    // Periodically compute RPM from FG pulses:
    //  - Every RPM_SAMPLE_MS, atomically read and clear pulse counter.
    //  - RPM = (pulses * 60) / PPR, if FG present and PPR > 0.
    //  - Safety: If FG present and motor is running but RPM=0 while clock>0,
    //            reduce target to 1/4 currentHz to mitigate a stall/missed feedback.
    //  - Optional telemetry dump to Serial if enabled.
    void sampleRPM()
    {
        uint32_t now = millis();
        if (now - lastRpmSample >= RPM_SAMPLE_MS)
        {
            // Atomically snapshot and reset the pulse count.
            noInterrupts();
            uint32_t p = fgPulses;
            fgPulses = 0;
            interrupts();

            if (prof.hasFG && prof.ppr > 0)
                rpm = (p * 60UL) / prof.ppr;
            else
                rpm = 0;

            lastRpmSample = now;

            // FG loss safety: detected when no pulses despite nonzero clock and running state.
            if (prof.hasFG && running)
            {
                if (rpm == 0 && currentHz > 0)
                {
                    targetHz = currentHz / 4;
                    setClock(targetHz);
#if DEBUG_MOTOR
                    Serial.println("FG loss detected - reducing speed");
#endif
                }
            }

            // Optional telemetry (RPM, clock, target, direction, LD status).
            if (telemetryOn)
            {
                Serial.print("RPM:");
                Serial.print(rpm);
                Serial.print(" Hz:");
                Serial.print(currentHz);
                Serial.print(" Target:");
                Serial.print(targetHz);
                Serial.print(" DIR:");
                Serial.print(dirCW ? "CW" : "CCW");
                Serial.print(" LD:");
                Serial.println(ldAlarm() ? "ALARM" : "OK");
            }
        }
    }

    // ---------------------- FG ISR ----------------------
    // Increment pulse count on each rising edge for RPM calculation.
    static void IRAM_ATTR isrFG();

    // ---------------------- System settings --------------
    // Enable/disable telemetry and persist to NVS.
    void setTelemetry(bool on)
    {
        telemetryOn = on;
        sysPrefs.begin("sys", false);
        sysPrefs.putBool("tele", telemetryOn);
        sysPrefs.end();

#if DEBUG_MOTOR
        Serial.print("Telemetry set to ");
        Serial.println(on ? "ON" : "OFF");
#endif
    }

    bool telemetry() const { return telemetryOn; }

    // Set UI language and persist to NVS.
    void setLanguage(Language L)
    {
        lang = L;
        sysPrefs.begin("sys", false);
        sysPrefs.putUChar("lang", (uint8_t)L);
        sysPrefs.end();

#if DEBUG_MOTOR
        Serial.print("Language set to ");
        Serial.println(L == LANG_EN ? "EN" : "ES");
#endif
    }

    Language getLanguage() const { return lang; }

    // ---------------------- Public fields ----------------
    // Expose current profile and key runtime state for UI/control modules.
    MotorProfile prof;
    bool        dirCW = true, brakeOn = false, enabled = true, running = false;
    uint32_t    targetHz = 1000, currentHz = 0, rpm = 0;

    // ---- Ramp state ----
    bool     rampActive = false;   // true while ramping toward targetHz
    uint32_t rampCurrentHz = 0;    // current ramp position

    // ---- Start timeout state ----
    bool     startTimeoutActive = false;
    uint32_t startTimeoutStart  = 0;

    // Update the ramp tick and start-timeout check.
    // Must be called frequently from the main loop (ideally every ~10 ms).
    void updateRamp()
    {
        uint32_t now = millis();

        // ---- Ramp tick ----
        if (rampActive && running)
        {
            if (now - lastRampTick >= RAMP_INTERVAL_MS)
            {
                lastRampTick = now;

                if (rampCurrentHz < targetHz)
                {
                    rampCurrentHz += RAMP_STEP_HZ;
                    if (rampCurrentHz > targetHz)
                        rampCurrentHz = targetHz;
                    setClock(rampCurrentHz);
                }
                else if (rampCurrentHz > targetHz)
                {
                    // Deceleration ramp
                    if (rampCurrentHz > RAMP_STEP_HZ)
                        rampCurrentHz -= RAMP_STEP_HZ;
                    else
                        rampCurrentHz = 0;
                    if (rampCurrentHz < targetHz)
                        rampCurrentHz = targetHz;
                    setClock(rampCurrentHz);
                }
                else
                {
                    // Reached target
                    rampActive = false;
#if DEBUG_MOTOR
                    Serial.println("Ramp complete");
#endif
                }
            }
        }

        // ---- Start timeout check ----
        if (startTimeoutActive && running)
        {
            if ((now - startTimeoutStart) >= START_TIMEOUT_MS)
            {
                // Only trigger if FG is configured and we still have no RPM
                if (prof.hasFG && rpm == 0)
                {
                    stop();
                    startTimeoutActive = false;
                    startTimeoutFired  = true;
#if DEBUG_MOTOR
                    Serial.println("Start timeout: no RPM detected, motor cut");
#endif
                }
                else
                {
                    // RPM received or no FG → cancel timeout monitoring
                    startTimeoutActive = false;
                    startTimeoutFired  = false;
                }
            }
            else if (prof.hasFG && rpm > 0)
            {
                // Got RPM before timeout expired → all good
                startTimeoutActive = false;
                startTimeoutFired  = false;
            }
        }
    }

    // Public flag: UI can read this to show a "no RPM / stall" warning.
    bool startTimeoutFired = false;

private:
    // Pulse counter updated from ISR; must be volatile.
    static volatile uint32_t fgPulses;

    // Timing for RPM sampling, preferences handle, and persisted flags.
    uint32_t    lastRpmSample = 0;
    uint32_t    lastRampTick  = 0;   // Last ramp tick timestamp
    Preferences sysPrefs;
    bool        telemetryOn = false;
    Language    lang = LANG_ES;
};

// -------- Static members & ISR definitions --------
volatile uint32_t MotorRuntime::fgPulses = 0;

void IRAM_ATTR MotorRuntime::isrFG()
{
    fgPulses++;
}
