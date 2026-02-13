#pragma once
#include <Arduino.h>
#include "Config.h"

class Buttons
{
public:
    void begin()
    {
        // Configure button pins with internal pull-ups.
        // Buttons are assumed to be active-LOW (pressed = LOW, released = HIGH).
        pinMode(PIN_BTN_UP, INPUT_PULLUP);
        pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
        pinMode(PIN_BTN_SEL, INPUT_PULLUP);

        // Read initial state after a short settling time to avoid false reads at boot.
        delay(50); // Stabilize inputs
        lastUp   = digitalRead(PIN_BTN_UP);
        lastDown = digitalRead(PIN_BTN_DOWN);
        lastSel  = digitalRead(PIN_BTN_SEL);

        // Initialize both "last" (instantaneous) and "stable" (debounced) states.
        stableUp   = lastUp;
        stableDown = lastDown;
        stableSel  = lastSel;

#if DEBUG_BUTTONS
        Serial.println("Buttons initialized");
        Serial.print("Initial states - UP:");
        Serial.print(stableUp);
        Serial.print(" DOWN:");
        Serial.print(stableDown);
        Serial.print(" SEL:");
        Serial.println(stableSel);
#endif
    }

    void poll()
    {
        // This should be called frequently (e.g., each loop()).
        // It performs debouncing, edge detection, and long-press evaluation for SEL.
        unsigned long now = millis();

        // Clear one-shot edge flags at the start of the polling cycle.
        // Each edge flag will be set once when a button transitions to pressed (LOW).
        upEdge = false;
        downEdge = false;
        selEdge = false;

        // Process all buttons with a common debouncing routine.
        // Only falling edges (HIGH -> LOW) generate an "edge" event.
        processButton(PIN_BTN_UP,   lastUp,   stableUp,   lastDebUp,   upEdge,   now, "UP");
        processButton(PIN_BTN_DOWN, lastDown, stableDown, lastDebDown, downEdge, now, "DOWN");
        processButton(PIN_BTN_SEL,  lastSel,  stableSel,  lastDebSel,  selEdge,  now, "SEL");

        // Long-press handling for the SELECT button:
        // - Start timing when SEL is held LOW.
        // - Once the press duration exceeds LONG_PRESS_MS, set longSel once
        //   and latch longSelTriggered to avoid re-triggering until released.
        if (stableSel == LOW)
        {
            if (selPressStart == 0)
            {
                // First frame the button is confirmed pressed (debounced)
                selPressStart = now;
            }
            else if (now - selPressStart > LONG_PRESS_MS)
            {
                if (!longSelTriggered)
                {
                    // Fire long-press event exactly once per hold
                    longSel = true;
                    longSelTriggered = true;
#if DEBUG_BUTTONS
                    Serial.println("SEL LONG press detected");
#endif
                }
            }
        }
        else
        {
            // Button released: reset long-press tracking state
            selPressStart = 0;
            longSelTriggered = false;
            longSel = false;
        }
    }

    // One-shot query: returns true exactly once when UP was just pressed (falling edge).
    bool upPressed()
    {
        bool result = upEdge;
        upEdge = false;     // consume the event
        return result;
    }

    // One-shot query: returns true exactly once when DOWN was just pressed (falling edge).
    bool downPressed()
    {
        bool result = downEdge;
        downEdge = false;   // consume the event
        return result;
    }

    // One-shot query: returns true exactly once when SEL was just pressed (falling edge).
    bool selPressed()
    {
        bool result = selEdge;
        selEdge = false;    // consume the event
        return result;
    }

    // One-shot query: returns true exactly once when a long-press on SEL is detected.
    // Resets the flag so subsequent calls return false until another long-press occurs.
    bool selLong()
    {
        bool result = longSel;
        longSel = false;    // consume the long-press event
        return result;
    }

    // Raw accessors for debounced, current-level states (active-LOW).
    bool rawUpLow()   const { return stableUp == LOW; }
    bool rawDownLow() const { return stableDown == LOW; }
    bool rawSelLow()  const { return stableSel == LOW; }

private:
    // Debounced button processing with falling-edge detection.
    //
    // Parameters:
    //  - pin:            GPIO to read.
    //  - last:           Last instantaneous reading (no debounce).
    //  - stable:         Debounced/confirmed state.
    //  - debounceTime:   Timestamp of the last change observed (for debounce window).
    //  - edge:           Output flag set to true once when a falling edge is confirmed.
    //  - now:            Current time in ms (millis()).
    //  - name:           Button name (for debug prints).
    //
    // Behavior:
    //  - Uses a 50 ms debounce window. When the raw reading changes, we mark the time.
    //  - If the reading remains different for >50 ms, we accept it as the new stable state.
    //  - We generate an edge ONLY on stable transition to LOW (pressed).
    void processButton(uint8_t pin, int &last, int &stable,
                       unsigned long &debounceTime, bool &edge, unsigned long now, const char* name)
    {
        int reading = digitalRead(pin);

        // If the instantaneous reading changed, (re)start the debounce timer.
        if (reading != last)
        {
            debounceTime = now;
        }

        // 50 ms debounce for stability against switch bounce and EMI.
        if (now - debounceTime > 50)
        {
            if (reading != stable)
            {
                // Accept the new stable state after the debounce interval.
                stable = reading;

                // Generate a one-shot event only on the falling edge (pressed = LOW).
                if (stable == LOW)
                {
                    edge = true;
#if DEBUG_BUTTONS
                    Serial.print("Button ");
                    Serial.print(name);
                    Serial.println(" pressed (edge)");
#endif
                }
            }
        }

        // Update the last instantaneous reading for next poll cycle.
        last = reading;
    }

    // Instantaneous last raw readings (non-debounced)
    int lastUp   = HIGH, lastDown = HIGH, lastSel = HIGH;

    // Debounced stable states
    int stableUp = HIGH, stableDown = HIGH, stableSel = HIGH;

    // Debounce timers (ms since boot)
    unsigned long lastDebUp = 0, lastDebDown = 0, lastDebSel = 0;

    // One-shot edge flags (true only for one poll cycle when a press is confirmed)
    bool upEdge = false, downEdge = false, selEdge = false;

    // Long-press one-shot flag for SELECT and its tracking variables
    bool longSel = false;
    unsigned long selPressStart = 0;
    bool longSelTriggered = false;
};
