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
#include "SimpleUnicode.h"

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
        case AUTOTEST:
            handleAutoTest();
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
        DIAG,
        AUTOTEST
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

    // Render the HOME screen with improved visual design using custom glyphs
    // New layout: Status header, progress bar, compact states, footer
    void drawHome()
    {
        if (!needRedraw)
            return;
        needRedraw = false;

        disp->firstPage();
        do
        {
            // ============ HEADER (Y: 0-12) ============
            // [●] RUNNING / [ ] STOPPED + RPM on the right
            disp->setFont(u8g2_font_6x12_tf);
            
            // Status icon (custom glyph) and text with brackets
            int xPos = 2;
            
            // Draw opening bracket [ (aligned with circle)
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawStr(xPos, 9, "[");
            xPos += 6; // Move past [
            
            // Draw status icon (filled or empty circle)
            if (motor->running)
                SimpleUnicode::drawFilledCircle(disp, xPos, 2);
            else
                SimpleUnicode::drawEmptyCircle(disp, xPos, 2);
            
            xPos += 8; // Move past the icon
            
            // Draw closing bracket ]
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawStr(xPos, 9, "]");
            xPos += 6; // Move past ]
            
            // Add space and status text
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawStr(xPos + 2, 10, motor->running ? "RUNNING" : "STOPPED");
            
            // RPM (right side, only if FG present)
            if (motor->prof.hasFG)
            {
                char rpmStr[16];
                snprintf(rpmStr, sizeof(rpmStr), "%lu", (unsigned long)motor->rpm);
                
                // Calculate position for right alignment
                disp->setFont(u8g2_font_6x12_tf);
                int rpmWidth = strlen(rpmStr) * 6; // Approximate text width
                int rpmX = 128 - rpmWidth - 10;    // Leave space for rotation icon
                
                disp->drawStr(rpmX, 10, rpmStr);
                
                // Draw rotation icon after the number
                SimpleUnicode::drawRotateArrow(disp, 128 - 10, 2);
            }
            
            // Separator line
            disp->drawLine(0, 13, 127, 13);
            
            // ============ SPEED BAR (Y: 16-38) ============
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawStr(2, 24, "Speed:");
            
            // Calculate progress bar (14 blocks)
            const int BAR_LENGTH = 14;
            int filledBlocks = 0;
            if (motor->prof.maxClockHz > 0)
            {
                filledBlocks = (motor->currentHz * BAR_LENGTH) / motor->prof.maxClockHz;
                if (filledBlocks > BAR_LENGTH) filledBlocks = BAR_LENGTH;
            }
            
            // Draw progress bar at Y=27 using custom glyphs
            SimpleUnicode::drawProgressBar(disp, 2, 27, BAR_LENGTH, filledBlocks);
            
            // Speed value (Hz) aligned to the right
            disp->setFont(u8g2_font_6x12_tf);
            char hzValue[16];
            snprintf(hzValue, sizeof(hzValue), "%luHz", (unsigned long)motor->currentHz);
            int hzWidth = strlen(hzValue) * 6;
            disp->drawStr(128 - hzWidth - 2, 35, hzValue);
            
            // ============ STATUS LINE (Y: 47) ============
            // Compact states: DIR + BRAKE + LD
            disp->setFont(u8g2_font_6x12_tf);
            
            int statusX = 2;
            int statusY = 47;
            
            // DIR (always shown)
            disp->drawStr(statusX, statusY, "DIR:");
            statusX += 24; // Move past "DIR:"
            
            // Draw direction arrow
            if (motor->dirCW)
                SimpleUnicode::drawArrowRight(disp, statusX, statusY - 8);
            else
                SimpleUnicode::drawArrowLeft(disp, statusX, statusY - 8);
            
            statusX += 14; // Move past arrow
            
            // BRAKE (if present)
            if (motor->prof.hasBrake)
            {
                disp->setFont(u8g2_font_6x12_tf);
                const char *brakeStatus = motor->brakeOn ? "BRK:ON" : "BRK:OFF";
                disp->drawStr(statusX, statusY, brakeStatus);
                statusX += strlen(brakeStatus) * 6 + 6;
            }
            
            // LD (if present)
            if (motor->prof.hasLD)
            {
                disp->setFont(u8g2_font_6x12_tf);
                disp->drawStr(statusX, statusY, "LD:");
                statusX += 18;
                
                // Draw check or X mark
                if (motor->ldAlarm())
                    SimpleUnicode::drawXMark(disp, statusX, statusY - 8);
                else
                    SimpleUnicode::drawCheckMark(disp, statusX, statusY - 8);
            }
            
            // ============ SEPARATOR LINE (Y: 49) ============
            disp->drawLine(0, 49, 127, 49);
            
            // ============ FOOTER (Y: 58) ============
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 58, S().footer_home);
            
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

        // LEFT: Go to diagnostics
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Going to DIAG");
#endif
            state = DIAG;
            needRedraw = true;
        }

        // RIGHT -> open main menu
        if (btn->rightPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] RIGHT: Going to MENU");
#endif
            state = MENU;
            menuIndex = 0;
            needRedraw = true;
            delay(150); // small debounce / UX pause
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
        items[n++] = S().m_autotest;  // ← AutoTest PRIMERO
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

        // LEFT: Back to HOME
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back to HOME from MENU");
#endif
            state = HOME;
            needRedraw = true;
            return;
        }

        // Short RIGHT: handle action by matching index in order
        if (btn->rightPressed())
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
            // AutoTest
            if (menuIndex == c++)
            {
                startAutoTest();
                return;
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
        }

        if (needRedraw)
        {
            drawMenuList(items, n);
            needRedraw = false;
        }
    }

    // Motor selection list.
    void handleSelectMotor()
    {
        int profileCount = pst->getCount();
        if (profileCount <= 0)
        {
            state = MENU;
            return;
        }

        // Build list: profile names only (no Back option needed)
        static char names[MAX_PROFILES][22];
        static const char *items[MAX_PROFILES];
        int n = 0;

        for (int i = 0; i < profileCount; i++)
        {
            String nm = pst->nameOf(i);
            nm.toCharArray(names[i], sizeof(names[i]));
            items[n++] = names[i];
        }

        // Navigation
        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // LEFT: Back to MENU
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back to MENU from SELECT_MOTOR");
#endif
            state = MENU;
            menuIndex = 0;
            needRedraw = true;
            return;
        }

        // RIGHT: Activate selected profile and go home
        if (btn->rightPressed())
        {
            pst->setActive(menuIndex);
            MotorProfile mp;
            pst->loadActive(mp);
            motor->applyProfile(mp);
            state = HOME;
            needRedraw = true;
            return;
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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
            if (btn->rightPressed())
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

            if (btn->rightPressed())
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

    // Settings main menu (Language, Telemetry).
    void handleSettings()
    {
        const char *items[2];
        int n = 0;
        items[n++] = S().s_language;
        items[n++] = S().s_telemetry;

        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // LEFT: Back to MENU
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back to MENU from SETTINGS");
#endif
            state = MENU;
            menuIndex = 0;
            needRedraw = true;
            return;
        }

        if (btn->rightPressed())
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
        }
        drawMenuList(items, n);
    }

    // Language selection (English, Español).
    void handleSettingsLang()
    {
        const char *items[2];
        int n = 0;
        items[n++] = S().s_lang_en;
        items[n++] = S().s_lang_es;

        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // LEFT: Back to SETTINGS
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back to SETTINGS from SETTINGS_LANG");
#endif
            state = SETTINGS;
            menuIndex = 0;
            needRedraw = true;
            return;
        }

        if (btn->rightPressed())
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
        }
        drawMenuList(items, n);
    }

    // Telemetry toggle screen (On/Off).
    void handleSettingsTele()
    {
        const char *items[1];
        int n = 0;
        items[n++] = motor->telemetry() ? S().s_telemetry_on : S().s_telemetry_off;

        // Navigation
        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // LEFT: Back to SETTINGS
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back to SETTINGS from SETTINGS_TELE");
#endif
            state = SETTINGS;
            menuIndex = 0;
            needRedraw = true;
            return;
        }

        if (btn->rightPressed())
        {
            // Toggle telemetry
            motor->setTelemetry(!motor->telemetry());
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("[UI] Telemetry toggled");
#endif
            // Update item label after toggling
            items[0] = motor->telemetry() ? S().s_telemetry_on : S().s_telemetry_off;
        }

        // Draw telemetry menu
        drawMenuList(items, n);
    }

    // About screen—press LEFT or RIGHT to go back to MENU.
    void handleAbout()
    {
        if (btn->leftPressed() || btn->rightPressed())
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
    // Press LEFT to exit back to HOME.
    void handleDiag()
    {
       // LEFT -> salir a HOME
        if (btn->leftPressed())
        {
            state = HOME;
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back to HOME from Diag");
#endif
            return;
        }

        // Always redraw live diagnostics
        char l1[32], l2[32], l3[32];
        snprintf(l1, sizeof(l1), "U:%d D:%d L:%d R:%d",
                 btn->rawUpLow() ? 1 : 0,
                 btn->rawDownLow() ? 1 : 0,
                 btn->rawLeftLow() ? 1 : 0,
                 btn->rawRightLow() ? 1 : 0);

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

    // -------------------- AutoTest Functions --------------------
    
    // Initialize and start the AutoTest sequence
    void startAutoTest()
    {
        // Save current state
        autoTestOriginalHz = motor->targetHz;
        autoTestOriginalDir = motor->dirCW;
        
        // Initialize test state
        autoTestCycle = 0;
        autoTestPhase = 0;
        autoTestAborted = false;
        autoTestStartTime = millis();
        
        // Stop motor if running
        if (motor->running)
            motor->stop();
        
        // Set to CW direction for first phase
        motor->setDirCW(true);
        
        // Enter autotest state
        state = AUTOTEST;
        needRedraw = true;
        
#if DEBUG_MOTOR
        Serial.println("[AutoTest] Starting test sequence");
#endif
    }
    
    // Handle AutoTest execution and display
    void handleAutoTest()
    {
        unsigned long elapsed = millis() - autoTestStartTime;
        
        // LEFT button to abort
        if (btn->leftPressed())
        {
            autoTestAborted = true;
            motor->stop();
            motor->targetHz = autoTestOriginalHz;
            motor->setDirCW(autoTestOriginalDir);
            state = HOME;
            needRedraw = true;
#if DEBUG_MOTOR
            Serial.println("[AutoTest] Aborted by user");
#endif
            return;
        }
        
        // Check LD alarm if available (safety stop)
        if (motor->prof.hasLD && motor->ldAlarm())
        {
            motor->stop();
            state = HOME;
            needRedraw = true;
#if DEBUG_MOTOR
            Serial.println("[AutoTest] ALARM detected - Test stopped");
#endif
            return;
        }
        
        // Phase timing and transitions
        bool phaseChanged = false;
        
        switch (autoTestPhase)
        {
        case 0: // CW direction test (8 seconds)
            if (elapsed < 3000)
            {
                // 0-3s: Low speed (30%)
                uint32_t lowSpeed = (motor->prof.maxClockHz * 30) / 100;
                if (!motor->running || motor->targetHz != lowSpeed)
                {
                    motor->targetHz = lowSpeed;
                    motor->start();
                    needRedraw = true;
                }
            }
            else if (elapsed < 8000)
            {
                // 3-8s: Normal speed (60%)
                uint32_t normalSpeed = (motor->prof.maxClockHz * 60) / 100;
                if (motor->targetHz != normalSpeed)
                {
                    motor->targetHz = normalSpeed;
                    motor->setClock(normalSpeed);
                    needRedraw = true;
                }
            }
            else
            {
                // Phase complete - pause 1s
                motor->stop();
                autoTestPhase = 1;
                autoTestStartTime = millis();
                phaseChanged = true;
                needRedraw = true;
            }
            break;
            
        case 1: // Pause 1 second
            if (elapsed >= 1000)
            {
                // Change to CCW
                motor->setDirCW(false);
                autoTestPhase = 2;
                autoTestStartTime = millis();
                phaseChanged = true;
                needRedraw = true;
            }
            break;
            
        case 2: // CCW direction test (8 seconds)
            if (elapsed < 3000)
            {
                // 0-3s: Low speed (30%)
                uint32_t lowSpeed = (motor->prof.maxClockHz * 30) / 100;
                if (!motor->running || motor->targetHz != lowSpeed)
                {
                    motor->targetHz = lowSpeed;
                    motor->start();
                    needRedraw = true;
                }
            }
            else if (elapsed < 8000)
            {
                // 3-8s: Normal speed (60%)
                uint32_t normalSpeed = (motor->prof.maxClockHz * 60) / 100;
                if (motor->targetHz != normalSpeed)
                {
                    motor->targetHz = normalSpeed;
                    motor->setClock(normalSpeed);
                    needRedraw = true;
                }
            }
            else
            {
                // Phase complete - pause 2s
                motor->stop();
                autoTestPhase = 3;
                autoTestStartTime = millis();
                phaseChanged = true;
                needRedraw = true;
            }
            break;
            
        case 3: // Pause 2 seconds between cycles
            if (elapsed >= 2000)
            {
                autoTestCycle++;
                if (autoTestCycle >= 3)
                {
                    // Test complete - restore and exit
                    motor->targetHz = autoTestOriginalHz;
                    motor->setDirCW(autoTestOriginalDir);
                    state = HOME;
                    needRedraw = true;
#if DEBUG_MOTOR
                    Serial.println("[AutoTest] Test completed successfully");
#endif
                    return;
                }
                else
                {
                    // Start next cycle
                    motor->setDirCW(true);
                    autoTestPhase = 0;
                    autoTestStartTime = millis();
                    phaseChanged = true;
                    needRedraw = true;
                }
            }
            break;
        }
        
        // Draw AutoTest screen
        if (needRedraw || phaseChanged)
        {
            needRedraw = false;
            
            disp->firstPage();
            do
            {
                // Header
                disp->setFont(u8g2_font_6x12_tf);
                disp->drawBox(0, 0, 128, 13);
                disp->setDrawColor(0);
                disp->drawStr(2, 10, "AUTO TEST");
                disp->setDrawColor(1);
                
                // Cycle info
                char cycleInfo[32];
                snprintf(cycleInfo, sizeof(cycleInfo), "Cycle: %d/3", autoTestCycle + 1);
                disp->setFont(u8g2_font_6x12_tf);
                disp->drawStr(2, 26, cycleInfo);
                
                // Phase info
                const char *phaseStr = "";
                switch (autoTestPhase)
                {
                case 0: phaseStr = "Phase: CW Test"; break;
                case 1: phaseStr = "Phase: Pause 1s"; break;
                case 2: phaseStr = "Phase: CCW Test"; break;
                case 3: phaseStr = "Phase: Pause 2s"; break;
                }
                disp->drawStr(2, 38, phaseStr);
                
                // Speed info
                char speedInfo[32];
                snprintf(speedInfo, sizeof(speedInfo), "Speed: %lu Hz", (unsigned long)motor->currentHz);
                disp->drawStr(2, 50, speedInfo);
                
                // Footer
                disp->setFont(u8g2_font_5x8_tf);
                disp->drawStr(2, 62, "LEFT to cancel");
                
            } while (disp->nextPage());
        }
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

    // AutoTest state variables
    unsigned long autoTestStartTime = 0;
    int autoTestCycle = 0;          // 0, 1, 2 (3 cycles total)
    int autoTestPhase = 0;          // 0=CW, 1=pause1s, 2=CCW, 3=pause2s
    uint32_t autoTestOriginalHz = 0;
    bool autoTestOriginalDir = true;
    bool autoTestAborted = false;
};
