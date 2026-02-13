#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include "Config.h"
#include "Strings_EN.h"
#include "Strings_ES.h"
#include "Buttons.h"
#include "Profiles.h"
#include "Motor.h"

class UI
{
public:
    // Initialize UI with display, input, storage and motor runtime references.
    // Shows a brief intro screen and adopts current language from MotorRuntime.
    void begin(U8G2 &d, Buttons &b, ProfileStore &store, MotorRuntime &m)
    {
        disp = &d;
        btn = &b;
        pst = &store;
        motor = &m;
        d.begin();
        drawIntro();
        lang = motor->getLanguage();
    }

    // Change current language and persist via MotorRuntime.
    void setLanguage(Language L)
    {
        lang = L;
        motor->setLanguage(L);
    }

    // Go to HOME state and request a redraw.
    void home()
    {
        state = HOME;
        needRedraw = true;
    }

    // Main UI update loop. Call this frequently from Arduino loop().
    // It dispatches to handlers/drawers based on the current state.
    void loop()
    {
        switch (state)
        {
        case HOME:
            drawHome();
            updateHome();
            break;
        case MENU:
            handleMenu();
            break;
        case SELECT_MOTOR:
            handleSelectMotor();
            break;
        case ADD_NAME:
        case ADD_Q_BRAKE:
        case ADD_Q_FG:
        case ADD_Q_LD:
        case ADD_Q_LD_LEVEL:
        case ADD_Q_STOP:
        case ADD_Q_STOP_LEVEL:
        case ADD_Q_ENABLE:
        case ADD_Q_ENABLE_LEVEL:
        case ADD_Q_PPR:
        case ADD_Q_MAXCLK:
        case ADD_SAVE:
            drawWizard();
            handleWizard();
            break;
        case SETTINGS:
            handleSettings();
            break;
        case SETTINGS_LANG:
            handleSettingsLang();
            break;
        case SETTINGS_TELE:
            handleSettingsTele();
            break;
        case ABOUT:
            handleAbout();
            break;
        case DIAG:
            handleDiag();
            break;
        }
    }

    // Enter diagnostics mode at boot if UP+DOWN are both pressed.
    void checkDiagAtBoot()
    {
        if (btn->rawUpLow() && btn->rawDownLow())
        {
            state = DIAG;
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("Entering DIAG mode");
#endif
        }
    }

private:
    // All UI states. Wizard steps are explicit to keep logic simple.
    enum State
    {
        HOME,
        MENU,
        SELECT_MOTOR,
        ADD_NAME,
        ADD_Q_BRAKE,
        ADD_Q_FG,
        ADD_Q_LD,
        ADD_Q_LD_LEVEL,
        ADD_Q_STOP,
        ADD_Q_STOP_LEVEL,
        ADD_Q_ENABLE,
        ADD_Q_ENABLE_LEVEL,
        ADD_Q_PPR,
        ADD_Q_MAXCLK,
        ADD_SAVE,
        SETTINGS,
        SETTINGS_LANG,
        SETTINGS_TELE,
        ABOUT,
        DIAG
    };

    // Resolve the current string table based on language.
    const Strings &S() const { return (lang == LANG_EN) ? STR_EN : STR_ES; }

    // Draw a double rounded frame (outer + inner) as a decorative container.
    void drawDoubleFrame()
    {
        // Outer rounded frame
        disp->drawRFrame(0, 0, 128, 64, 3);

        // Inner rounded frame (inset by 2 px)
        disp->drawRFrame(2, 2, 124, 60, 2);
    }

    // Generic header: title bar with inverted background.
    void header(const char *title)
    {
        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            disp->drawStr(2, 10, title);
            disp->setDrawColor(1);
        } while (disp->nextPage());
    }

    // Initial splash screen shown at UI startup.
    void drawIntro()
    {
        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_logisoso20_tr);
            disp->drawStr(4, 30, "Fran-Byte");
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawStr(4, 52, "Motor Tester v2");
        } while (disp->nextPage());
        delay(900);
    }

    // Render the HOME screen (header, RPM/speed, status lines, footer hints).
    // Redraw is gated by 'needRedraw' for efficiency.
    void drawHome()
    {
        if (!needRedraw)
            return;
        needRedraw = false;

        disp->firstPage();
        do
        {
            // Header with motor name (or default) and ON/OFF status
            disp->setFont(u8g2_font_6x12_tf);
            const char *title = (motor->prof.name[0] ? motor->prof.name : S().hdr_home);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);

            // Motor name (left)
            disp->drawStr(2, 10, title);

            // ON/OFF status (right)
            const char *status = motor->running ? "ON" : "OFF";
            int statusWidth = strlen(status) * 6; // 6 px per character at 6x12 font
            disp->drawStr(128 - statusWidth - 2, 10, status);

            disp->setDrawColor(1);

            int y = 22; // Initial Y for content

            // RPM (only if FG is available)
            disp->setFont(u8g2_font_6x12_tf);
            if (motor->prof.hasFG)
            {
                char line[32];
                snprintf(line, sizeof(line), "%s %lu", S().rpm, (unsigned long)motor->rpm);
                disp->drawStr(2, y, line);
                y += 11;
            }

            // Target speed (Hz), always shown
            char spd[40];
            snprintf(spd, sizeof(spd), "%s %lu Hz", S().speed, (unsigned long)motor->targetHz);
            disp->drawStr(2, y, spd);
            y += 11;

            // Secondary lines (smaller font 5x8): DIR + optional BRAKE/ENABLE/LD
            disp->setFont(u8g2_font_5x8_tf);

            // Collect up to 4 status items to flow across lines.
            char statusItems[4][32]; // DIR + BRAKE + ENABLE + LD
            int itemCount = 0;

            // DIR is always first
            char dirline[32];
            snprintf(dirline, sizeof(dirline), "%s %s", S().dir, (motor->dirCW ? S().cw : S().ccw));
            strcpy(statusItems[itemCount], dirline);
            itemCount++;

            // BRAKE (if present)
            if (motor->prof.hasBrake)
            {
                snprintf(statusItems[itemCount], sizeof(statusItems[itemCount]),
                         "%s %s", S().brake, (motor->brakeOn ? S().on : S().off));
                itemCount++;
            }

            // ENABLE (if present)
            if (motor->prof.hasEnable)
            {
                snprintf(statusItems[itemCount], sizeof(statusItems[itemCount]),
                         "%s %s", S().enable, (motor->enabled ? S().on : S().off));
                itemCount++;
            }

            // LD (if present), show as ✓ (OK) or ✗ (ALARM)
            if (motor->prof.hasLD)
            {
                const char *ldSymbol = motor->ldAlarm() ? "✗" : "✓";
                snprintf(statusItems[itemCount], sizeof(statusItems[itemCount]),
                         "%s: %s", S().ld, ldSymbol);
                itemCount++;
            }

            // Flow items with wrapping across lines
            int currentX = 2;       // Start X
            int maxLineWidth = 126; // Max width per line
            int currentLineY = y;   // Start Y

            for (int i = 0; i < itemCount; i++)
            {
                int itemWidth = strlen(statusItems[i]) * 5; // 5 px per char at 5x8 font

                // Wrap to next line if it doesn't fit (except first item)
                if (currentX + itemWidth > maxLineWidth && i > 0)
                {
                    currentLineY += 9; // advance line (approx line height)
                    currentX = 2;
                }

                // Draw item
                disp->drawStr(currentX, currentLineY, statusItems[i]);

                // Advance X with spacing
                currentX += itemWidth + 8;
            }

            // Footer help/hints aligned at bottom
            y = currentLineY + 9;
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 63, S().footer_home);
        } while (disp->nextPage());
    }

    // Handle input on HOME screen: step speed, open menu, start/stop on long press.
    void updateHome()
    {
        static unsigned long lastSpeedChange = 0;
        const unsigned long SPEED_DELAY = 150; // rate-limit for speed changes
        unsigned long now = millis();

        // UP: increase speed (coarse step strategy in MotorRuntime)
        if (btn->upPressed())
        {
            if (now - lastSpeedChange > SPEED_DELAY)
            {
                motor->stepSpeedUp();
                needRedraw = true;
                lastSpeedChange = now;
#if DEBUG_SPEED
                Serial.println("[UI] UP button pressed");
#endif
            }
        }

        // DOWN: decrease speed
        if (btn->downPressed())
        {
            if (now - lastSpeedChange > SPEED_DELAY)
            {
                motor->stepSpeedDown();
                needRedraw = true;
                lastSpeedChange = now;
#if DEBUG_SPEED
                Serial.println("[UI] DOWN button pressed");
#endif
            }
        }

        // SELECT short -> open main menu
        if (btn->selPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] SELECT short: Going to MENU");
#endif
            state = MENU;
            menuIndex = 0;
            needRedraw = true;
            delay(150); // small debounce / UX pause
        }

        // SELECT long -> start/stop motor
        if (btn->selLong())
        {
#if DEBUG_MOTOR
            Serial.print("[UI] SELECT long: ");
            Serial.println(motor->running ? "STOP" : "START");
#endif
            if (motor->running)
                motor->stop();
            else
                motor->start();
            needRedraw = true;
            delay(200); // UX pause
        }
    }

    // Render a generic, scrollable, framed menu list with a header and footer hints.
    void drawMenuList(const char **items, int n)
    {
        if (n == 0)
            return;

        disp->firstPage();
        do
        {
            // Decorative double rounded frame
            drawDoubleFrame();

            disp->setFont(u8g2_font_6x12_tf);
            // Header bar with rounded background
            disp->drawRBox(4, 4, 120, 13, 2);
            disp->setDrawColor(0);
            disp->drawStr(6, 14, S().menu);
            disp->setDrawColor(1);

            int lineHeight = 10;
            int maxVisibleLines = 3; // keep small to preserve margins

            // Keep scroll window covering the selected index
            if (menuIndex < menuScroll)
                menuScroll = menuIndex;
            if (menuIndex >= menuScroll + maxVisibleLines)
                menuScroll = menuIndex - maxVisibleLines + 1;

            int startY = 28; // content Y origin

            for (int i = 0; i < maxVisibleLines; i++)
            {
                int idx = menuScroll + i;
                if (idx >= n)
                    break;

                int y = startY + (i * lineHeight);

                if (idx == menuIndex)
                {
                    // Highlight the selected row
                    disp->drawRBox(6, y - 8, 116, lineHeight, 2);
                    disp->setDrawColor(0);
                    disp->drawStr(8, y, items[idx]);
                    disp->setDrawColor(1);
                }
                else
                {
                    disp->drawStr(8, y, items[idx]);
                }
            }

            // Footer hints
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(6, 60, S().footer_menu);

        } while (disp->nextPage());
    }

    // Main menu: build dynamic items according to runtime (running, brake presence, profiles).
    // Short SELECT executes action, long SELECT is intentionally disabled in menus.
    void handleMenu()
    {
        const char *items[12];
        int n = 0;
        items[n++] = motor->running ? S().m_stop : S().m_start;
        items[n++] = motor->dirCW ? S().m_set_ccw : S().m_set_cw;
        if (motor->prof.hasBrake)
            items[n++] = motor->brakeOn ? S().m_brake_off : S().m_brake_on;
        if (pst->getCount() > 0)
            items[n++] = S().m_select_motor;
        items[n++] = S().m_add_motor;
        if (pst->getCount() > 0)
            items[n++] = S().m_delete_active;
        items[n++] = S().m_settings;
        items[n++] = S().m_about;
        items[n++] = S().m_back; // Back/return to HOME

        // Navigation
        if (btn->upPressed() && menuIndex > 0)
        {
            menuIndex--;
            needRedraw = true;
        }

        if (btn->downPressed() && menuIndex < n - 1)
        {
            menuIndex++;
            needRedraw = true;
        }

        // Short SELECT: handle action by matching index in order
        if (btn->selPressed())
        {
            delay(100); // small UX pause
            int c = 0;

            if (menuIndex == c++)
            {
                if (motor->running)
                    motor->stop();
                else
                    motor->start();
                state = HOME;
                needRedraw = true;
                return;
            }
            if (menuIndex == c++)
            {
                motor->toggleDir();
                state = HOME;
                needRedraw = true;
                return;
            }
            if (motor->prof.hasBrake)
            {
                if (menuIndex == c++)
                {
                    motor->toggleBrake();
                    state = HOME;
                    needRedraw = true;
                    return;
                }
            }
            if (pst->getCount() > 0)
            {
                if (menuIndex == c++)
                {
                    state = SELECT_MOTOR;
                    menuIndex = pst->getActiveIndex();
                    needRedraw = true;
                    return;
                }
            }
            if (menuIndex == c++)
            {
                enterAddWizard();
                return;
            }
            if (pst->getCount() > 0)
            {
                if (menuIndex == c++)
                {
                    int idx = pst->getActiveIndex();
                    pst->remove(idx);
                    MotorProfile mp;
                    if (!pst->loadActive(mp))
                        mp.setDefaults();
                    motor->applyProfile(mp);
                    state = HOME;
                    needRedraw = true;
                    return;
                }
            }
            if (menuIndex == c++)
            {
                state = SETTINGS;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
            if (menuIndex == c++)
            {
                state = ABOUT;
                needRedraw = true;
                return;
            }
            // Back to HOME
            if (menuIndex == c++)
            {
                state = HOME;
                needRedraw = true;
                return;
            }
        }

        if (needRedraw)
        {
            drawMenuList(items, n);
            needRedraw = false;
        }
    }

    // Motor selection list with a trailing "Back" entry.
    void handleSelectMotor()
    {
        int profileCount = pst->getCount();
        if (profileCount <= 0)
        {
            state = MENU;
            return;
        }

        // Build list: profile names + final Back option
        static char names[MAX_PROFILES + 1][22];
        static const char *items[MAX_PROFILES + 1];
        int n = 0;

        for (int i = 0; i < profileCount; i++)
        {
            String nm = pst->nameOf(i);
            nm.toCharArray(names[i], sizeof(names[i]));
            items[n++] = names[i];
        }
        // Append "Back"
        strcpy(names[profileCount], S().m_back);
        items[n++] = names[profileCount];

        // Navigation
        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // Short SELECT
        if (btn->selPressed())
        {
            // Last item is Back
            if (menuIndex == profileCount)
            {
                state = MENU;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
            // Any other item: activate profile and go home
            else
            {
                pst->setActive(menuIndex);
                MotorProfile mp;
                pst->loadActive(mp);
                motor->applyProfile(mp);
                state = HOME;
                needRedraw = true;
                return;
            }
        }

        drawMenuList(items, n);
    }

    // Prepare temporary profile and buffers for the Add Profile wizard.
    void enterAddWizard()
    {
        memset(&tmp, 0, sizeof(tmp));
        tmp.setDefaults();
        memset(editName, 0, sizeof(editName));
        editPos = 0;
        state = ADD_NAME;
        needRedraw = true;
    }

    // Advance wizard state machine according to current step and flags.
    void wizardNext()
    {
        if (state == ADD_NAME)
            state = ADD_Q_BRAKE;
        else if (state == ADD_Q_BRAKE)
            state = ADD_Q_FG;
        else if (state == ADD_Q_FG)
            state = ADD_Q_LD;
        else if (state == ADD_Q_LD)
            state = tmp.hasLD ? ADD_Q_LD_LEVEL : ADD_Q_STOP;
        else if (state == ADD_Q_LD_LEVEL)
            state = ADD_Q_STOP;
        else if (state == ADD_Q_STOP)
            state = tmp.hasStop ? ADD_Q_STOP_LEVEL : ADD_Q_ENABLE;
        else if (state == ADD_Q_STOP_LEVEL)
            state = ADD_Q_ENABLE;
        else if (state == ADD_Q_ENABLE)
            state = tmp.hasEnable ? ADD_Q_ENABLE_LEVEL : ADD_Q_PPR;
        else if (state == ADD_Q_ENABLE_LEVEL)
            state = ADD_Q_PPR;
        else if (state == ADD_Q_PPR)
            state = ADD_Q_MAXCLK;
        else if (state == ADD_Q_MAXCLK)
            state = ADD_SAVE;
        needRedraw = true;
    }

    // Draw current wizard step. For ADD_NAME we always redraw for blinking cursor.
    void drawWizard()
    {
        // For name editing we refresh every frame to show blinking cursor/END box
        if (state == ADD_NAME)
        {
            needRedraw = true;
        }

        if (!needRedraw)
            return;
        needRedraw = false;

        char line1[32], line2[32], hint[64];
        line1[0] = 0;
        line2[0] = 0;
        strcpy(hint, S().hint_yesno);

        if (state == ADD_NAME)
        {
            strcpy(line1, S().w_name);

            // Prepare line2 from edit buffer.
            const char END_MARKER = 0x7F;
            strcpy(line2, editName);

            // If current position holds END_MARKER, show special END box instead of a char
            if (editName[editPos] == END_MARKER)
            {
                // Temporarily terminate for drawing before the END box
                line2[editPos] = 0;
            }

            strcpy(hint, S().hint_text);
        }
        else if (state == ADD_Q_BRAKE)
        {
            strcpy(line1, S().w_has_brake);
            strcpy(line2, tmp.hasBrake ? S().yes : S().no);
        }
        else if (state == ADD_Q_FG)
        {
            strcpy(line1, S().w_has_fg);
            strcpy(line2, tmp.hasFG ? S().yes : S().no);
        }
        else if (state == ADD_Q_LD)
        {
            strcpy(line1, S().w_has_ld);
            strcpy(line2, tmp.hasLD ? S().yes : S().no);
        }
        else if (state == ADD_Q_LD_LEVEL)
        {
            strcpy(line1, S().w_ld_active);
            strcpy(line2, tmp.ldActiveLow ? S().low : S().high);
            strcpy(hint, S().hint_choice);
        }
        else if (state == ADD_Q_STOP)
        {
            strcpy(line1, S().w_has_stop);
            strcpy(line2, tmp.hasStop ? S().yes : S().no);
        }
        else if (state == ADD_Q_STOP_LEVEL)
        {
            strcpy(line1, S().w_stop_active);
            strcpy(line2, tmp.stopActiveHigh ? S().high : S().low);
            strcpy(hint, S().hint_choice);
        }
        else if (state == ADD_Q_ENABLE)
        {
            strcpy(line1, S().w_has_enable);
            strcpy(line2, tmp.hasEnable ? S().yes : S().no);
        }
        else if (state == ADD_Q_ENABLE_LEVEL)
        {
            strcpy(line1, S().w_enable_active);
            strcpy(line2, tmp.enableActiveHigh ? S().high : S().low);
            strcpy(hint, S().hint_choice);
        }
        else if (state == ADD_Q_PPR)
        {
            strcpy(line1, S().w_ppr);
            snprintf(line2, sizeof(line2), "%d", tmp.ppr);
            strcpy(hint, S().hint_number);
        }
        else if (state == ADD_Q_MAXCLK)
        {
            strcpy(line1, S().w_maxclk);
            snprintf(line2, sizeof(line2), "%lu", (unsigned long)tmp.maxClockHz);
            strcpy(hint, S().hint_number);
        }
        else if (state == ADD_SAVE)
        {
            strcpy(line1, S().w_save);
            strcpy(line2, wizardSaveChoice ? S().yes : S().no);
            strcpy(hint, S().hint_yesno);
        }

        // Draw wizard screen content
        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            // Header bar for wizard
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            disp->drawStr(2, 10, S().m_add_motor);
            disp->setDrawColor(1);

            // Question/prompt
            disp->drawStr(2, 28, line1);

            // Value / editable line with cursor handling in ADD_NAME
            if (state == ADD_NAME)
            {
                const char END_MARKER = 0x7F;

                // Draw current text line
                disp->drawStr(2, 42, line2);

                // If current char is END marker, draw a small "END" box
                if (editName[editPos] == END_MARKER)
                {
                    int endX = 2 + (editPos * 6);

                    // Small rectangle with "END" inside (using 4x6 font)
                    disp->drawFrame(endX, 34, 18, 10);
                    disp->setFont(u8g2_font_4x6_tr);
                    disp->drawStr(endX + 1, 42, "END");
                    disp->setFont(u8g2_font_6x12_tf);
                }
                else
                {
                    // Blinking underline cursor below the current position
                    int cursorX = 2 + (editPos * 6);
                    if ((millis() / 500) % 2 == 0) // blink every 500 ms
                    {
                        disp->drawLine(cursorX, 44, cursorX + 5, 44);
                    }
                }
            }
            else
            {
                // Non-edit steps: just draw the value line
                disp->drawStr(2, 42, line2);
            }

            // Footer hint
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 62, hint);
        } while (disp->nextPage());
    }

    // Handle input for all wizard steps (name editor, toggles, numeric fields, save).
    void handleWizard()
    {
        if (state == ADD_NAME)
        {
            // Inline name editor with a special END marker to finish entry.
            // Alphabet cycle: A-Z -> 0-9 -> space -> '-' -> '_' -> END -> (wrap)
            const char END_MARKER = 0x7F;

            if (btn->upPressed())
            {
                // Initialize with 'A' on first edit
                if (editName[editPos] == 0)
                    editName[editPos] = 'A';
                else
                {
                    editName[editPos]++;
                    // Forward cycle
                    if (editName[editPos] == 'Z' + 1)
                        editName[editPos] = '0';
                    else if (editName[editPos] == '9' + 1)
                        editName[editPos] = ' ';
                    else if (editName[editPos] == ' ' + 1)
                        editName[editPos] = '-';
                    else if (editName[editPos] == '-' + 1)
                        editName[editPos] = '_';
                    else if (editName[editPos] == '_' + 1)
                        editName[editPos] = END_MARKER;
                    else if (editName[editPos] > END_MARKER)
                        editName[editPos] = 'A';
                }
                needRedraw = true;
            }

            if (btn->downPressed())
            {
                // Initialize with 'A' on first edit
                if (editName[editPos] == 0)
                    editName[editPos] = 'A';
                else
                {
                    editName[editPos]--;
                    // Reverse cycle across allowed chars and END marker
                    if (editName[editPos] < 'A' && editName[editPos] != END_MARKER && editName[editPos] != '_' && editName[editPos] != '-' && editName[editPos] != ' ')
                    {
                        editName[editPos] = END_MARKER;
                    }
                    else if (editName[editPos] == END_MARKER - 1)
                        editName[editPos] = '_';
                    else if (editName[editPos] == '_' - 1)
                        editName[editPos] = '-';
                    else if (editName[editPos] == '-' - 1)
                        editName[editPos] = ' ';
                    else if (editName[editPos] == ' ' - 1)
                        editName[editPos] = '9';
                    else if (editName[editPos] == '0' - 1)
                        editName[editPos] = 'Z';
                }
                needRedraw = true;
            }

            // SELECT: advance; if current char is END, finalize the name
            if (btn->selPressed())
            {
                if (editName[editPos] == END_MARKER)
                {
                    // Remove END marker and finish
                    editName[editPos] = 0;

                    // Default fallback if empty
                    if (editName[0] == 0)
                    {
                        strcpy(editName, "Motor");
                    }

                    strncpy(tmp.name, editName, sizeof(tmp.name));
                    wizardNext();
#if DEBUG_BUTTONS
                    Serial.print("[Wizard] Name finalized: ");
                    Serial.println(tmp.name);
#endif
                    return;
                }

                // Ensure current char is initialized
                if (editName[editPos] == 0)
                    editName[editPos] = 'A';

                // Move to next char position, or finalize if buffer end reached
                if (editPos < 18)
                {
                    editPos++;
#if DEBUG_BUTTONS
                    Serial.print("[Wizard] Moved to position: ");
                    Serial.println(editPos);
#endif
                }
                else
                {
                    strncpy(tmp.name, editName, sizeof(tmp.name));
                    wizardNext();
                }
                needRedraw = true;
            }
            return;
        }

        // The remaining wizard states toggle or adjust values with UP/DOWN,
        // and advance with SELECT.
        if (state == ADD_Q_BRAKE)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasBrake = !tmp.hasBrake;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_FG)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasFG = !tmp.hasFG;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_LD)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasLD = !tmp.hasLD;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_LD_LEVEL)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.ldActiveLow = !tmp.ldActiveLow;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_STOP)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasStop = !tmp.hasStop;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_STOP_LEVEL)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.stopActiveHigh = !tmp.stopActiveHigh;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_ENABLE)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasEnable = !tmp.hasEnable;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_ENABLE_LEVEL)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.enableActiveHigh = !tmp.enableActiveHigh;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_PPR)
        {
            if (btn->upPressed())
            {
                tmp.ppr++;
                needRedraw = true;
            }
            if (btn->downPressed() && tmp.ppr > 1)
            {
                tmp.ppr--;
                needRedraw = true;
            }
            if (btn->selPressed())
                wizardNext();
            return;
        }
        if (state == ADD_Q_MAXCLK)
        {
            if (btn->upPressed())
            {
                tmp.maxClockHz += 1000;
                needRedraw = true;
            }
            if (btn->downPressed() && tmp.maxClockHz > 1000)
            {
                tmp.maxClockHz -= 1000;
                needRedraw = true;
            }
            if (btn->selPressed())
                wizardNext();
            return;
        }
        if (state == ADD_SAVE)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                wizardSaveChoice = !wizardSaveChoice;
                needRedraw = true;
            }

            if (btn->selPressed())
            {
                if (wizardSaveChoice)
                {
                    // Save profile to storage and make it active
                    pst->append(tmp);
                    pst->setActive(pst->getCount() - 1);
                    MotorProfile mp;
                    pst->loadActive(mp);
                    motor->applyProfile(mp);
                }
                // If "NO", just exit without saving
                state = HOME;
                needRedraw = true;
                wizardSaveChoice = true; // reset for next time
                return;
            }
        }
    }

    // Settings main menu (Language, Telemetry, Back).
    void handleSettings()
    {
        const char *items[4];
        int n = 0;
        items[n++] = S().s_language;
        items[n++] = S().s_telemetry;
        items[n++] = S().m_back; // Back option

        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        if (btn->selPressed())
        {
            int c = 0;
            if (menuIndex == c++)
            {
                state = SETTINGS_LANG;
                menuIndex = (lang == LANG_EN ? 0 : 1);
                needRedraw = true;
                return;
            }
            if (menuIndex == c++)
            {
                state = SETTINGS_TELE;
                needRedraw = true;
                return;
            }
            // Back to main menu
            if (menuIndex == c++)
            {
                state = MENU;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
        }
        drawMenuList(items, n);
    }

    // Language selection (English, Español, Back).
    void handleSettingsLang()
    {
        const char *items[3];
        int n = 0;
        items[n++] = S().s_lang_en;
        items[n++] = S().s_lang_es;
        items[n++] = S().m_back; // Back option

        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        if (btn->selPressed())
        {
            if (menuIndex == 0) // English
            {
                lang = LANG_EN;
                motor->setLanguage(lang);
                state = HOME;
                needRedraw = true;
                return;
            }
            if (menuIndex == 1) // Español
            {
                lang = LANG_ES;
                motor->setLanguage(lang);
                state = HOME;
                needRedraw = true;
                return;
            }
            if (menuIndex == 2) // Back
            {
                state = SETTINGS;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
        }
        drawMenuList(items, n);
    }

    // Telemetry toggle screen (On/Off) with Back.
    void handleSettingsTele()
    {
        const char *items[2];
        int n = 0;
        items[n++] = motor->telemetry() ? S().s_telemetry_on : S().s_telemetry_off;
        items[n++] = S().m_back; // Back option

        // Navigation
        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        if (btn->selPressed())
        {
            if (menuIndex == 0) // Toggle telemetry
            {
                motor->setTelemetry(!motor->telemetry());
                needRedraw = true;
#if DEBUG_BUTTONS
                Serial.println("[UI] Telemetry toggled");
#endif
                // Update item label after toggling
                items[0] = motor->telemetry() ? S().s_telemetry_on : S().s_telemetry_off;
            }
            else if (menuIndex == 1) // Back
            {
                state = SETTINGS;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
        }

        // Draw telemetry menu
        drawMenuList(items, n);
    }

    // About screen—press SELECT to go back to MENU.
    void handleAbout()
    {
        if (btn->selPressed())
        {
            state = MENU;
            menuIndex = 0;
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("[UI] Back to MENU from About");
#endif
            return;
        }

        // Always redraw (static content but cheap to render)
        char build[24];
        snprintf(build, sizeof(build), "%s %s", S().about_build, __DATE__);

        disp->firstPage();
        do
        {
            // Framed header
            drawDoubleFrame();

            disp->setFont(u8g2_font_6x12_tf);
            disp->drawRBox(4, 4, 120, 13, 2);
            disp->setDrawColor(0);
            disp->drawStr(6, 14, S().about_title);
            disp->setDrawColor(1);
            disp->drawStr(8, 30, S().about_author);
            disp->drawStr(8, 42, S().about_version);
            disp->drawStr(8, 54, build);
            // No footer hint; SELECT simply returns
        } while (disp->nextPage());
    }

    // Diagnostics screen: live button states, LD, RPM, frequency, direction.
    // Hold LEFT to exit back to HOME.
    void handleDiag()
    {
       // RIGHT long -> salir a HOME
        if (btn->rightLongPress))
        {
            state = HOME;
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("[UI] Back to HOME from Diag");
#endif
            return;
        }

        // Always redraw live diagnostics
        char l1[32], l2[32], l3[32];
        snprintf(l1, sizeof(l1), "UP:%d DN:%d SEL:%d",
                 btn->rawUpLow() ? 1 : 0,
                 btn->rawDownLow() ? 1 : 0,
                 btn->rawSelLow() ? 1 : 0);

        int ld = digitalRead(PIN_LD);
        snprintf(l2, sizeof(l2), "LD:%d FG-rpm:%lu",
                 (motor->prof.ldActiveLow ? (ld == LOW) : (ld == HIGH)) ? 1 : 0,
                 (unsigned long)motor->rpm);

        snprintf(l3, sizeof(l3), "Hz:%lu DIR:%s",
                 (unsigned long)motor->currentHz,
                 motor->dirCW ? "CW" : "CCW");

        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            disp->drawStr(2, 10, S().diag_title);
            disp->setDrawColor(1);
            disp->drawStr(2, 26, l1);
            disp->drawStr(2, 38, l2);
            disp->drawStr(2, 50, l3);
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 62, S().diag_hint);
        } while (disp->nextPage());
    }

    // -------------------- Dependencies & State --------------------
    U8G2 *disp = nullptr;
    Buttons *btn = nullptr;
    ProfileStore *pst = nullptr;
    MotorRuntime *motor = nullptr;

    State state = HOME;
    bool needRedraw = true;
    int menuIndex = 0;
    int menuScroll = 0;
    Language lang = LANG_ES;

    // Wizard temp storage and editor buffers
    MotorProfile tmp;
    char editName[20] = {0};
    int editPos = 0;
    bool wizardSaveChoice = true;
};
