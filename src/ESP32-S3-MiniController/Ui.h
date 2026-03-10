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

        // First boot: no admin password set → enter password setup wizard.
        if (!pst->hasAdminPassword())
        {
            memset(adminPwBuf, 0, sizeof(adminPwBuf));
            memset(adminConfirmBuf, 0, sizeof(adminConfirmBuf));
            adminPwPos   = 0;
            adminMismatch = false;
            state        = ADMIN_SET_PW;
            needRedraw   = true;
        }
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
        case MANUAL:
            handleManual();
            break;
        case DIAG:
            handleDiag();
            break;
        case AUTOTEST:
            handleAutoTest();
            break;
        case ADMIN_SET_PW:
        case ADMIN_CONFIRM_PW:
            needRedraw = true;  // force redraw every frame for blinking cursor
            handleAdminSetPassword();
            break;
        case ADMIN_LOGIN:
            needRedraw = true;  // force redraw every frame for blinking cursor
            handleAdminLogin();
            break;
        case ADMIN_PROFILE_MENU:
            handleAdminProfileMenu();
            break;
        case ADMIN_DELETE_LIST:
            handleAdminDeleteList();
            break;
        case USER_PANEL:
            handleUserPanel();
            break;
        case USER_DELETE_LIST:
            handleUserDeleteList();
            break;
        case CONFIRM:
            handleConfirm();
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
        MANUAL,
        DIAG,
        AUTOTEST,
        // ---- Admin password setup (first boot) ----
        ADMIN_SET_PW,       // Enter new admin password for the first time
        ADMIN_CONFIRM_PW,   // Confirm the new admin password
        // ---- Admin login (for protected actions) ----
        ADMIN_LOGIN,        // Enter password to authenticate as admin
        // ---- Admin profile management ----
        ADMIN_PROFILE_MENU, // Admin panel menu (add/delete/mode)
        ADMIN_DELETE_LIST,  // Admin delete submenu: all profiles
        USER_PANEL,         // User panel menu (add/delete/mode)
        USER_DELETE_LIST,   // User delete submenu: only [U] profiles
        CONFIRM             // Generic yes/no confirmation screen
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
            
            // Admin/User session mode indicator — top right, next to Speed label
            {
                const char *badge = adminSessionActive ? "[A]" : "[U]";
                disp->setFont(u8g2_font_5x8_tf);
                disp->drawStr(128 - 16, 24, badge);
                disp->setFont(u8g2_font_6x12_tf);
            }
            
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

            // If a start-timeout fired, show stall warning instead of normal footer
            if (motor->startTimeoutFired)
            {
                disp->drawStr(2, 58, (lang == LANG_EN) ? "! STALL / NO RPM !" : "! PARADO / SIN RPM !");
            }
            else
            {
                disp->drawStr(2, 58, S().footer_home);
            }
            
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

    // Helper: returns "[A]" or "[U]" depending on current session mode.
    const char *modeBadge() const { return adminSessionActive ? "[A]" : "[U]"; }

    // Render a generic, scrollable, framed menu list with a header and footer hints.
    // Title is explicit so each screen can show its own label + mode badge.
    void drawMenuList(const char **items, int n, const char *title = nullptr, const char *footer = nullptr, int selectedIndex = -1)
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

            // Draw title (left side)
            const char *t = title ? title : S().menu;
            disp->drawStr(6, 14, t);

            // Mode badge — right side of header bar, small font
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(108, 13, modeBadge());
            disp->setFont(u8g2_font_6x12_tf);

            disp->setDrawColor(1);

            // If footer is explicitly "", skip it and use the extra space for a 4th item row
            bool showFooter = !(footer && footer[0] == '\0');
            int lineHeight     = 10;
            int maxVisibleLines = showFooter ? 3 : 4;
            int sel = (selectedIndex >= 0) ? selectedIndex : menuIndex;

            // Keep scroll window covering the selected index
            if (sel < menuScroll)
                menuScroll = sel;
            if (sel >= menuScroll + maxVisibleLines)
                menuScroll = sel - maxVisibleLines + 1;

            int startY = 28;

            for (int i = 0; i < maxVisibleLines; i++)
            {
                int idx = menuScroll + i;
                if (idx >= n)
                    break;

                int y = startY + (i * lineHeight);

                if (idx == sel)
                {
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

            // Footer hints (omitted when footer="")
            if (showFooter)
            {
                disp->setFont(u8g2_font_5x8_tf);
                disp->drawStr(6, 60, footer ? footer : S().footer_menu);
            }

        } while (disp->nextPage());
    }

    // Main menu: build dynamic items according to runtime (running, brake presence, profiles).
    // Short SELECT executes action, long SELECT is intentionally disabled in menus.
    void handleMenu()
    {
        const char *items[7];
        int n = 0;
        items[n++] = motor->running ? S().m_stop : S().m_start;
        items[n++] = motor->dirCW ? S().m_set_ccw : S().m_set_cw;
        if (motor->prof.hasBrake)
            items[n++] = motor->brakeOn ? S().m_brake_off : S().m_brake_on;
        items[n++] = S().m_autotest;
        if (pst->getCount() > 0)
            items[n++] = S().m_select_motor;
        items[n++] = adminSessionActive
            ? ((lang == LANG_EN) ? "Admin Panel" : "Panel Admin")
            : ((lang == LANG_EN) ? "User Panel"  : "Panel User");

        if (btn->upPressed() && menuIndex > 0)        { menuIndex--; needRedraw = true; }
        if (btn->downPressed() && menuIndex < n - 1)  { menuIndex++; needRedraw = true; }

        if (btn->leftPressed())
        {
            state = HOME; needRedraw = true; return;
        }

        if (btn->rightPressed())
        {
            delay(100);
            int c = 0;

            if (menuIndex == c++) // Start / Stop
            {
                if (motor->running) motor->stop(); else motor->start();
                state = HOME; needRedraw = true; return;
            }
            if (menuIndex == c++) // Direction
            {
                motor->toggleDir();
                state = HOME; needRedraw = true; return;
            }
            if (motor->prof.hasBrake)
            {
                if (menuIndex == c++) // Brake
                {
                    motor->toggleBrake();
                    state = HOME; needRedraw = true; return;
                }
            }
            if (menuIndex == c++) // AutoTest
            {
                startAutoTest(); return;
            }
            if (pst->getCount() > 0)
            {
                if (menuIndex == c++) // Select Motor
                {
                    state = SELECT_MOTOR;
                    menuIndex = pst->getActiveIndex();
                    menuScroll = 0;
                    needRedraw = true; return;
                }
            }
            if (menuIndex == c++) // Panel (Admin or User)
            {
                if (adminSessionActive)
                    enterAdminProfilePanel();
                else
                    enterUserPanel();
                return;
            }
        }

        if (needRedraw)
        {
            drawMenuList(items, n, S().menu, "");
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

        // Build list — ALL profiles, any mode can select any motor
        char names[MAX_PROFILES][22];
        const char *items[MAX_PROFILES];
        int n = 0;

        for (int i = 0; i < profileCount; i++)
        {
            String nm = pst->nameOf(i);
            snprintf(names[n], sizeof(names[n]), "%s [%s]",
                     nm.c_str(), pst->isAdminProfile(i) ? "A" : "U");
            items[n] = names[n];
            n++;
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

        drawMenuList(items, n, S().m_select_motor, "");
    }

    // Prepare temporary profile and buffers for the Add Profile wizard.
    void enterAddWizard(State returnTo = HOME)
    {
        memset(&tmp, 0, sizeof(tmp));
        tmp.setDefaults();
        memset(editName, 0, sizeof(editName));
        editPos = 0;
        wizardReturnState = returnTo;
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
                    // Flag follows the current session mode
                    tmp.isAdminProfile = adminSessionActive;
                    pst->append(tmp);
                    pst->setActive(pst->getCount() - 1);
                    MotorProfile mp;
                    pst->loadActive(mp);
                    motor->applyProfile(mp);
                }
                // If "NO", just exit without saving
                state = wizardReturnState;
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
        drawMenuList(items, n, S().s_title, "");
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

        // LEFT: Back to panel (or SETTINGS if entered from there)
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back from SETTINGS_LANG");
#endif
            state = panelReturnState;
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
        drawMenuList(items, n, S().s_language, "");
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

        // LEFT: Back to panel
        if (btn->leftPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] LEFT: Back from SETTINGS_TELE");
#endif
            state = panelReturnState;
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
        drawMenuList(items, n, S().s_telemetry, "");
    }

    // About screen—press LEFT or RIGHT to go back to panel.
    void handleAbout()
    {
        if (btn->leftPressed() || btn->rightPressed())
        {
            state = panelReturnState;
            menuIndex = 0;
            needRedraw = true;
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

    // -------------------- Manual Functions --------------------
    
    // Display user manual with multiple pages
    void handleManual()
    {
        const int TOTAL_PAGES = (lang == LANG_EN) ? 5 : 5;  // 5 pages in both languages
        
        // Navigation
        if (btn->upPressed() && manualPage > 0)
        {
            manualPage--;
            needRedraw = true;
        }
        
        if (btn->downPressed() && manualPage < TOTAL_PAGES - 1)
        {
            manualPage++;
            needRedraw = true;
        }
        
        // LEFT to exit back to panel
        if (btn->leftPressed())
        {
            state = panelReturnState;
            menuIndex = 0;
            needRedraw = true;
            return;
        }
        
        if (!needRedraw)
            return;
            
        needRedraw = false;
        
        disp->firstPage();
        do
        {
            // Header
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            char header[32];
            snprintf(header, sizeof(header), "%s %d/%d", S().manual_title, manualPage + 1, TOTAL_PAGES);
            disp->drawStr(2, 10, header);
            disp->setDrawColor(1);
            
            // Content — font 5x8, baseline starts at y, glyph occupies [y-7 .. y+1]
            // Screen height = 64. Safe baseline range: 8 .. 62 (glyph fully visible).
            disp->setFont(u8g2_font_5x8_tf);
            int y   = 20;   // first baseline
            int spc = 8;    // 8px step: 6 lines land at y=20,28,36,44,52,60 (all within 64px)

            #define MLINE(txt) disp->drawStr(2, y, txt); y += spc;

            if (lang == LANG_EN)
            {
                switch (manualPage)
                {
                case 0:
                    MLINE("BASIC CONTROLS:")
                    MLINE("UP/DN: Change speed")
                    MLINE("RIGHT: Open menu")
                    MLINE("LEFT: Diagnostics")
                    MLINE("In Menu: RIGHT=OK")
                    MLINE("LEFT=Back")
                    break;
                case 1:
                    MLINE("MENU OPTIONS:")
                    MLINE("Start/Stop motor")
                    MLINE("Change direction")
                    MLINE("Brake control")
                    MLINE("Auto Test")
                    MLINE("Select profiles")
                    break;
                case 2:
                    MLINE("AUTO TEST:")
                    MLINE("3 cycles, CW+CCW")
                    MLINE("Tests low & normal")
                    MLINE("speed in both")
                    MLINE("directions. Stops")
                    MLINE("on LD alarm.")
                    break;
                case 3:
                    MLINE("PROFILES:")
                    MLINE("Add Motor: Create")
                    MLINE("new profile with")
                    MLINE("custom settings")
                    MLINE("Select Motor: Pick")
                    MLINE("from saved profiles")
                    break;
                case 4:
                    MLINE("SAFETY:")
                    MLINE("LD: Alarm signal")
                    MLINE("FG: RPM feedback")
                    MLINE("If FG fails, speed")
                    MLINE("auto-reduces to")
                    MLINE("25% for safety.")
                    break;
                }
            }
            else
            {
                switch (manualPage)
                {
                case 0:
                    MLINE("CONTROLES BASICOS:")
                    MLINE("UP/DN: Cambiar vel.")
                    MLINE("RIGHT: Abrir menu")
                    MLINE("LEFT: Diagnostico")
                    MLINE("En Menu: RIGHT=OK")
                    MLINE("LEFT=Atras")
                    break;
                case 1:
                    MLINE("OPCIONES MENU:")
                    MLINE("Arrancar/Parar")
                    MLINE("Cambiar direccion")
                    MLINE("Control de freno")
                    MLINE("Auto Test")
                    MLINE("Seleccionar perfil")
                    break;
                case 2:
                    MLINE("AUTO TEST:")
                    MLINE("3 ciclos, CW+CCW")
                    MLINE("Prueba velocidad")
                    MLINE("baja y normal en")
                    MLINE("ambas direcciones.")
                    MLINE("Para si hay alarma.")
                    break;
                case 3:
                    MLINE("PERFILES:")
                    MLINE("Anadir Motor: Crear")
                    MLINE("perfil con ajustes")
                    MLINE("personalizados")
                    MLINE("Select Motor: Elegir")
                    MLINE("perfil guardado")
                    break;
                case 4:
                    MLINE("SEGURIDAD:")
                    MLINE("LD: Senal de alarma")
                    MLINE("FG: Realimentacion")
                    MLINE("Si FG falla, la")
                    MLINE("velocidad se reduce")
                    MLINE("al 25% seguridad")
                    break;
                }
            }

            #undef MLINE
            
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

    // -------------------- Admin Password Setup (first boot) --------------------

    // Shared character editor used for password entry (re-uses same cycle as name editor).
    // Populates adminPwBuf (SET_PW step) or adminConfirmBuf (CONFIRM_PW step).
    void handleAdminSetPassword()
    {
        const char END_MARKER = 0x7F;
        char *buf    = (state == ADMIN_SET_PW) ? adminPwBuf : adminConfirmBuf;
        int  maxLen  = ADMIN_PW_MAX_LEN;

        // UP / DOWN: cycle through A-Z, 0-9, END
        if (btn->upPressed())
        {
            if (buf[adminPwPos] == 0)         buf[adminPwPos] = 'A';
            else if (buf[adminPwPos] == 'Z')  buf[adminPwPos] = '0';
            else if (buf[adminPwPos] == '9')  buf[adminPwPos] = END_MARKER;
            else if (buf[adminPwPos] > END_MARKER) buf[adminPwPos] = 'A';
            else                              buf[adminPwPos]++;
            needRedraw = true;
        }
        if (btn->downPressed())
        {
            if (buf[adminPwPos] == 0)         buf[adminPwPos] = 'A';
            else if (buf[adminPwPos] == 'A')  buf[adminPwPos] = END_MARKER;
            else if (buf[adminPwPos] == '0')  buf[adminPwPos] = 'Z';
            else if (buf[adminPwPos] == END_MARKER) buf[adminPwPos] = '9';
            else                              buf[adminPwPos]--;
            needRedraw = true;
        }

        // RIGHT: advance or finalize
        if (btn->rightPressed())
        {
            if (buf[adminPwPos] == END_MARKER || adminPwPos >= maxLen - 1)
            {
                buf[adminPwPos] = 0; // terminate
                if (buf[0] == 0) return; // must have at least 1 char

                if (state == ADMIN_SET_PW)
                {
                    // Move to confirm step
                    adminPwPos = 0;
                    adminMismatch = false;
                    state = ADMIN_CONFIRM_PW;
                    needRedraw = true;
                }
                else
                {
                    // Compare set vs confirm
                    if (strcmp(adminPwBuf, adminConfirmBuf) == 0)
                    {
                        pst->setAdminPassword(adminPwBuf);
                        adminSessionActive = true; // logged in automatically after setup
                        state = HOME;
                        needRedraw = true;
#if DEBUG_MOTOR
                        Serial.println("[Admin] Password set successfully");
#endif
                    }
                    else
                    {
                        // Mismatch: restart from SET_PW
                        adminMismatch = true;
                        memset(adminPwBuf, 0, sizeof(adminPwBuf));
                        memset(adminConfirmBuf, 0, sizeof(adminConfirmBuf));
                        adminPwPos = 0;
                        state = ADMIN_SET_PW;
                        needRedraw = true;
                    }
                }
                return;
            }

            if (buf[adminPwPos] == 0) buf[adminPwPos] = 'A';
            if (adminPwPos < maxLen - 1) adminPwPos++;
            needRedraw = true;
        }

        // Draw password setup screen
        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            const char *hdr = (state == ADMIN_SET_PW)
                ? ((lang == LANG_EN) ? "SET ADMIN PW" : "CREAR PW ADMIN")
                : ((lang == LANG_EN) ? "CONFIRM PW"   : "CONFIRMAR PW");
            disp->drawStr(2, 10, hdr);
            disp->setDrawColor(1);

            if (adminMismatch)
            {
                disp->drawStr(2, 28, (lang == LANG_EN) ? "Mismatch! Retry:" : "No coincide! Re:");
            }
            else
            {
                disp->drawStr(2, 28, (lang == LANG_EN) ? "Password:" : "Contrasena:");
            }

            // Draw asterisks for already-confirmed chars
            char stars[ADMIN_PW_MAX_LEN + 2];
            for (int i = 0; i < adminPwPos; i++) stars[i] = '*';
            stars[adminPwPos] = 0;
            int starsX = 2;
            disp->drawStr(starsX, 42, stars);
            int curX = starsX + adminPwPos * 6; // 6px per char (6x12 font)

            if (buf[adminPwPos] == END_MARKER)
            {
                // Draw END box — same style as name editor
                disp->drawFrame(curX, 34, 18, 10);
                disp->setFont(u8g2_font_4x6_tr);
                disp->drawStr(curX + 1, 42, "END");
                disp->setFont(u8g2_font_6x12_tf);
            }
            else
            {
                // Show current char being edited (visible, not masked)
                char cur[2] = { buf[adminPwPos] ? buf[adminPwPos] : '_', 0 };
                disp->drawStr(curX, 42, cur);
                // Blinking underline cursor
                if ((millis() / 500) % 2 == 0)
                    disp->drawLine(curX, 44, curX + 5, 44);
            }

            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 62, (lang == LANG_EN) ? "UP/DN=Char R=Next/END" : "UP/DN=Caracter R=Sig");
        } while (disp->nextPage());
    }

    // -------------------- Admin Login --------------------

    void enterAdminLogin(State returnTo)
    {
        adminLoginReturn = returnTo;
        memset(adminLoginBuf, 0, sizeof(adminLoginBuf));
        adminLoginPos    = 0;
        adminLoginFailed = false;
        state            = ADMIN_LOGIN;
        needRedraw       = true;
    }

    void handleAdminLogin()
    {
        const char END_MARKER = 0x7F;
        char *buf   = adminLoginBuf;
        int  maxLen = ADMIN_PW_MAX_LEN;

        if (btn->upPressed())
        {
            if (buf[adminLoginPos] == 0)               buf[adminLoginPos] = 'A';
            else if (buf[adminLoginPos] == 'Z')        buf[adminLoginPos] = '0';
            else if (buf[adminLoginPos] == '9')        buf[adminLoginPos] = END_MARKER;
            else if (buf[adminLoginPos] > END_MARKER)  buf[adminLoginPos] = 'A';
            else                                       buf[adminLoginPos]++;
            needRedraw = true;
        }
        if (btn->downPressed())
        {
            if (buf[adminLoginPos] == 0)               buf[adminLoginPos] = 'A';
            else if (buf[adminLoginPos] == 'A')        buf[adminLoginPos] = END_MARKER;
            else if (buf[adminLoginPos] == '0')        buf[adminLoginPos] = 'Z';
            else if (buf[adminLoginPos] == END_MARKER) buf[adminLoginPos] = '9';
            else                                       buf[adminLoginPos]--;
            needRedraw = true;
        }

        if (btn->leftPressed())
        {
            // Cancel login → go back
            state = adminLoginReturn;
            menuIndex = 0;
            needRedraw = true;
            return;
        }

        if (btn->rightPressed())
        {
            if (buf[adminLoginPos] == END_MARKER || adminLoginPos >= maxLen - 1)
            {
                buf[adminLoginPos] = 0;
                // Verify
                if (pst->checkAdminPassword(buf))
                {
                    adminSessionActive = true;
                    adminLoginFailed   = false;
                    // Execute pending action
                    if (pendingAdminAction == ADMIN_ACT_DELETE)
                    {
                        pst->remove(pendingProfileIdx);
                        MotorProfile mp;
                        if (!pst->loadActive(mp)) mp.setDefaults();
                        motor->applyProfile(mp);
                        state = HOME;
                    }
                    else if (pendingAdminAction == ADMIN_ACT_MODE)
                    {
                        // Mode switched to Admin — go to main menu
                        state = MENU;
                        menuIndex = 0;
                    }
                    else
                    {
                        // ADMIN_ACT_PANEL
                        enterAdminProfilePanel();
                    }
                    needRedraw = true;
                    return;
                }
                else
                {
                    adminLoginFailed = true;
                    memset(adminLoginBuf, 0, sizeof(adminLoginBuf));
                    adminLoginPos = 0;
                    needRedraw = true;
                    return;
                }
            }
            if (buf[adminLoginPos] == 0) buf[adminLoginPos] = 'A';
            if (adminLoginPos < maxLen - 1) adminLoginPos++;
            needRedraw = true;
        }

        // Draw login screen
        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            disp->drawStr(2, 10, (lang == LANG_EN) ? "ADMIN LOGIN" : "ACCESO ADMIN");
            disp->setDrawColor(1);

            if (adminLoginFailed)
                disp->drawStr(2, 26, (lang == LANG_EN) ? "Wrong password!" : "Clave incorrecta!");
            else
                disp->drawStr(2, 26, (lang == LANG_EN) ? "Enter password:" : "Introduzca clave:");

            // Draw asterisks for already-confirmed chars
            char stars[ADMIN_PW_MAX_LEN + 2];
            for (int i = 0; i < adminLoginPos; i++) stars[i] = '*';
            stars[adminLoginPos] = 0;
            int starsX = 2;
            disp->drawStr(starsX, 42, stars);
            int curX = starsX + adminLoginPos * 6;

            if (buf[adminLoginPos] == END_MARKER)
            {
                // Draw END box — same style as name editor
                disp->drawFrame(curX, 34, 18, 10);
                disp->setFont(u8g2_font_4x6_tr);
                disp->drawStr(curX + 1, 42, "END");
                disp->setFont(u8g2_font_6x12_tf);
            }
            else
            {
                // Show current char being edited (visible)
                char cur[2] = { buf[adminLoginPos] ? buf[adminLoginPos] : '_', 0 };
                disp->drawStr(curX, 42, cur);
                // Blinking underline cursor
                if ((millis() / 500) % 2 == 0)
                    disp->drawLine(curX, 44, curX + 5, 44);
            }

            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 62, (lang == LANG_EN) ? "R=Confirm L=Cancel" : "R=OK L=Cancelar");
        } while (disp->nextPage());
    }

    // -------------------- User Panel --------------------
    // Shows only [U] profiles. Any user can delete them.

    void enterUserPanel()
    {
        userPanelIndex = 0;
        menuScroll     = 0;
        state          = USER_PANEL;
        needRedraw     = true;
    }

    void handleUserPanel()
    {
        const char *items[7];
        items[0] = (lang == LANG_EN) ? "Add Motor"     : "Anadir Motor";
        items[1] = (lang == LANG_EN) ? "Delete Motor"  : "Borrar Motor";
        items[2] = (lang == LANG_EN) ? "Language"      : "Idioma";
        items[3] = (lang == LANG_EN) ? "Telemetry"     : "Telemetria";
        items[4] = (lang == LANG_EN) ? "Manual"        : "Manual";
        items[5] = (lang == LANG_EN) ? "About"         : "Acerca de";
        items[6] = (lang == LANG_EN) ? "-> Admin mode" : "-> Modo Admin";
        const int N = 7;

        if (btn->upPressed()   && userPanelIndex > 0)     { userPanelIndex--; needRedraw = true; }
        if (btn->downPressed() && userPanelIndex < N - 1) { userPanelIndex++; needRedraw = true; }

        if (btn->leftPressed())
        {
            state = MENU; menuIndex = 0; needRedraw = true; return;
        }

        if (btn->rightPressed())
        {
            if (userPanelIndex == 0) { enterAddWizard(USER_PANEL); return; }
            if (userPanelIndex == 1) { deleteListIndex = 0; menuScroll = 0; state = USER_DELETE_LIST; needRedraw = true; return; }
            if (userPanelIndex == 2) { panelReturnState = USER_PANEL; state = SETTINGS_LANG; menuIndex = (lang == LANG_EN ? 0 : 1); needRedraw = true; return; }
            if (userPanelIndex == 3) { panelReturnState = USER_PANEL; state = SETTINGS_TELE; needRedraw = true; return; }
            if (userPanelIndex == 4) { panelReturnState = USER_PANEL; state = MANUAL; manualPage = 0; needRedraw = true; return; }
            if (userPanelIndex == 5) { panelReturnState = USER_PANEL; state = ABOUT; needRedraw = true; return; }
            if (userPanelIndex == 6) { enterConfirm(CONF_MODE_TO_ADMIN); return; }
        }

        if (needRedraw)
        {
            char title[20];
            snprintf(title, sizeof(title), "%s [U]",
                     (lang == LANG_EN) ? "USER PANEL" : "PANEL USER");
            drawMenuList(items, N, title, "", userPanelIndex);
            needRedraw = false;
        }
    }

    // User delete list — shows ONLY [U] profiles
    void handleUserDeleteList()
    {
        static int   uMap[MAX_PROFILES];
        static char  names[MAX_PROFILES][22];
        static const char *items[MAX_PROFILES];
        int n = 0;
        for (int i = 0; i < pst->getCount(); i++)
        {
            if (!pst->isAdminProfile(i))
            {
                String nm = pst->nameOf(i);
                snprintf(names[n], sizeof(names[n]), "%s [U]", nm.c_str());
                items[n] = names[n];
                uMap[n]  = i;
                n++;
            }
        }

        if (btn->upPressed()   && deleteListIndex > 0)    { deleteListIndex--; needRedraw = true; }
        if (btn->downPressed() && deleteListIndex < n - 1) { deleteListIndex++; needRedraw = true; }

        if (btn->leftPressed())
        {
            state = USER_PANEL; needRedraw = true; return;
        }

        if (btn->rightPressed() && n > 0)
        {
            enterConfirm(CONF_DELETE_USER, uMap[deleteListIndex]); return;
        }

        if (!needRedraw) return;
        needRedraw = false;

        if (n == 0)
        {
            drawEmptyList((lang == LANG_EN) ? "No [U] motors" : "Sin motores [U]",
                          (lang == LANG_EN) ? "DEL MOTORS [U]" : "BORRAR MOT [U]");
            return;
        }

        if (deleteListIndex < menuScroll)       menuScroll = deleteListIndex;
        if (deleteListIndex >= menuScroll + 3)  menuScroll = deleteListIndex - 2;

        char title[20];
        snprintf(title, sizeof(title), "%s", (lang == LANG_EN) ? "DEL MOTORS [U]" : "BORRAR MOT [U]");
        drawMenuList(items, n, title, "", deleteListIndex);
    }

    // -------------------- Admin Profile Panel --------------------

    void enterAdminProfilePanel()
    {
        adminPanelIndex = 0;
        menuScroll = 0;
        state      = ADMIN_PROFILE_MENU;
        needRedraw = true;
    }

    void handleAdminProfileMenu()
    {
        const char *items[7];
        items[0] = (lang == LANG_EN) ? "Add Motor"    : "Anadir Motor";
        items[1] = (lang == LANG_EN) ? "Delete Motor" : "Borrar Motor";
        items[2] = (lang == LANG_EN) ? "Language"     : "Idioma";
        items[3] = (lang == LANG_EN) ? "Telemetry"    : "Telemetria";
        items[4] = (lang == LANG_EN) ? "Manual"       : "Manual";
        items[5] = (lang == LANG_EN) ? "About"        : "Acerca de";
        items[6] = (lang == LANG_EN) ? "-> User mode" : "-> Modo User";
        const int N = 7;

        if (btn->upPressed()   && adminPanelIndex > 0)     { adminPanelIndex--; needRedraw = true; }
        if (btn->downPressed() && adminPanelIndex < N - 1) { adminPanelIndex++; needRedraw = true; }

        if (btn->leftPressed())
        {
            state = MENU; menuIndex = 0; needRedraw = true; return;
        }

        if (btn->rightPressed())
        {
            if (adminPanelIndex == 0) { enterAddWizard(ADMIN_PROFILE_MENU); return; }
            if (adminPanelIndex == 1) { deleteListIndex = 0; menuScroll = 0; state = ADMIN_DELETE_LIST; needRedraw = true; return; }
            if (adminPanelIndex == 2) { panelReturnState = ADMIN_PROFILE_MENU; state = SETTINGS_LANG; menuIndex = (lang == LANG_EN ? 0 : 1); needRedraw = true; return; }
            if (adminPanelIndex == 3) { panelReturnState = ADMIN_PROFILE_MENU; state = SETTINGS_TELE; needRedraw = true; return; }
            if (adminPanelIndex == 4) { panelReturnState = ADMIN_PROFILE_MENU; state = MANUAL; manualPage = 0; needRedraw = true; return; }
            if (adminPanelIndex == 5) { panelReturnState = ADMIN_PROFILE_MENU; state = ABOUT; needRedraw = true; return; }
            if (adminPanelIndex == 6) { enterConfirm(CONF_MODE_TO_USER); return; }
        }

        if (needRedraw)
        {
            char title[20];
            snprintf(title, sizeof(title), "%s [A]",
                     (lang == LANG_EN) ? "ADMIN PANEL" : "PANEL ADMIN");
            drawMenuList(items, N, title, "", adminPanelIndex);
            needRedraw = false;
        }
    }

    // Admin delete list — shows ALL profiles (admin + user)
    void handleAdminDeleteList()
    {
        int cnt = pst->getCount();
        static char  names[MAX_PROFILES][24];
        static const char *items[MAX_PROFILES];
        int n = 0;
        for (int i = 0; i < cnt; i++)
        {
            String nm = pst->nameOf(i);
            snprintf(names[i], sizeof(names[i]), "%s [%s]",
                     nm.c_str(), pst->isAdminProfile(i) ? "A" : "U");
            items[n++] = names[i];
        }

        if (btn->upPressed()   && deleteListIndex > 0)    { deleteListIndex--; needRedraw = true; }
        if (btn->downPressed() && deleteListIndex < n - 1) { deleteListIndex++; needRedraw = true; }

        if (btn->leftPressed())
        {
            state = ADMIN_PROFILE_MENU; needRedraw = true; return;
        }

        if (btn->rightPressed() && n > 0)
        {
            enterConfirm(CONF_DELETE_ADMIN, deleteListIndex); return;
        }

        if (!needRedraw) return;
        needRedraw = false;

        if (n == 0)
        {
            drawEmptyList((lang == LANG_EN) ? "No motors" : "Sin motores",
                          (lang == LANG_EN) ? "DEL MOTORS [A]" : "BORRAR MOT [A]");
            return;
        }

        // Scroll
        if (deleteListIndex < menuScroll)         menuScroll = deleteListIndex;
        if (deleteListIndex >= menuScroll + 3)    menuScroll = deleteListIndex - 2;

        char title[20];
        snprintf(title, sizeof(title), "%s", (lang == LANG_EN) ? "DEL MOTORS [A]" : "BORRAR MOT [A]");
        drawMenuList(items, n, title, "", deleteListIndex);
    }

    // Small helper: draw an empty-list screen with a title and centered message.
    void drawEmptyList(const char *msg, const char *title)
    {
        disp->firstPage();
        do
        {
            drawDoubleFrame();
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawRBox(4, 4, 120, 13, 2);
            disp->setDrawColor(0);
            disp->drawStr(6, 14, title);
            disp->setDrawColor(1);
            disp->drawStr(8, 36, msg);
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(6, 60, (lang == LANG_EN) ? "L=Back" : "L=Atras");
        } while (disp->nextPage());
    }

    // -------------------- Generic Confirmation Screen --------------------
    // Call enterConfirm() with a message and two callbacks (confirm / cancel).
    // RIGHT = confirm, LEFT = cancel. UP/DOWN toggle the selection.

    enum ConfirmAction {
        CONF_DELETE_USER,    // Delete a [U] profile from user panel
        CONF_DELETE_ADMIN,   // Delete any profile from admin panel
        CONF_MODE_TO_USER,   // Switch session mode Admin → User
        CONF_MODE_TO_ADMIN   // Switch session mode User → Admin (then asks PW)
    };

    void enterConfirm(ConfirmAction action, int profileIdx = -1)
    {
        confirmAction   = action;
        confirmProfIdx  = profileIdx;
        confirmChoice   = false; // default = NO
        state           = CONFIRM;
        needRedraw      = true;
    }

    void handleConfirm()
    {
        if (btn->upPressed() || btn->downPressed())
        {
            confirmChoice = !confirmChoice;
            needRedraw = true;
        }

        if (btn->leftPressed()) // cancel
        {
            if (confirmAction == CONF_DELETE_USER)
                state = USER_DELETE_LIST;
            else if (confirmAction == CONF_DELETE_ADMIN)
                state = ADMIN_DELETE_LIST;
            else
                state = ADMIN_PROFILE_MENU;
            needRedraw = true;
            return;
        }

        if (btn->rightPressed())
        {
            if (!confirmChoice) // NO → cancel
            {
                if (confirmAction == CONF_DELETE_USER)
                    state = USER_DELETE_LIST;
                else if (confirmAction == CONF_DELETE_ADMIN)
                    state = ADMIN_DELETE_LIST;
                else
                    state = ADMIN_PROFILE_MENU;
                needRedraw = true;
                return;
            }

            // YES → execute
            if (confirmAction == CONF_DELETE_USER || confirmAction == CONF_DELETE_ADMIN)
            {
                pst->remove(confirmProfIdx);
                MotorProfile mp;
                if (!pst->loadActive(mp)) mp.setDefaults();
                motor->applyProfile(mp);
                if (deleteListIndex > 0) deleteListIndex--;
                state = (confirmAction == CONF_DELETE_USER) ? USER_DELETE_LIST : ADMIN_DELETE_LIST;
            }
            else if (confirmAction == CONF_MODE_TO_USER)
            {
                adminSessionActive = false;
                state = MENU;
                menuIndex = 0;
            }
            else if (confirmAction == CONF_MODE_TO_ADMIN)
            {
                pendingAdminAction = ADMIN_ACT_MODE;
                enterAdminLogin(MENU);
                return;
            }
            needRedraw = true;
            return;
        }

        if (!needRedraw) return;
        needRedraw = false;

        // Build message line depending on action
        char msg[24];
        if (confirmAction == CONF_DELETE_USER || confirmAction == CONF_DELETE_ADMIN)
        {
            strncpy(msg, (lang == LANG_EN) ? "DELETE?" : "BORRAR?", sizeof(msg));
        }
        else if (confirmAction == CONF_MODE_TO_USER)
        {
            strncpy(msg, (lang == LANG_EN) ? "Switch to USER?" : "Cambiar a USER?", sizeof(msg));
        }
        else
        {
            strncpy(msg, (lang == LANG_EN) ? "Switch to ADMIN?" : "Cambiar a ADMIN?", sizeof(msg));
        }

        disp->firstPage();
        do
        {
            drawDoubleFrame();
            disp->setFont(u8g2_font_6x12_tf);

            // Header
            disp->drawRBox(4, 4, 120, 13, 2);
            disp->setDrawColor(0);
            disp->drawStr(6, 14, msg);
            disp->setDrawColor(1);

            // YES / NO options centered
            const char *yesStr = (lang == LANG_EN) ? "YES" : "SI";
            const char *noStr  = "NO";

            // NO on left, YES on right — highlight selected
            int yesX = 72, noX = 20, optY = 38;

            if (confirmChoice) // YES highlighted
            {
                disp->drawRBox(yesX - 4, optY - 10, 30, 14, 3);
                disp->setDrawColor(0);
                disp->drawStr(yesX, optY, yesStr);
                disp->setDrawColor(1);
                disp->drawStr(noX, optY, noStr);
            }
            else // NO highlighted
            {
                disp->drawRBox(noX - 4, optY - 10, 28, 14, 3);
                disp->setDrawColor(0);
                disp->drawStr(noX, optY, noStr);
                disp->setDrawColor(1);
                disp->drawStr(yesX, optY, yesStr);
            }

            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(6, 60, (lang == LANG_EN) ? "UP/DN=Toggle R=OK L=Cancel" : "UP/DN=Cambiar R=OK L=Cancel");
        } while (disp->nextPage());
    }
    U8G2 *disp = nullptr;
    Buttons *btn = nullptr;
    ProfileStore *pst = nullptr;
    MotorRuntime *motor = nullptr;

    State state = HOME;
    bool needRedraw = true;
    int menuIndex = 0;
    int menuScroll = 0;
    int manualPage = 0;
    Language lang = LANG_ES;

    // Wizard temp storage and editor buffers
    MotorProfile tmp;
    char editName[20] = {0};
    int editPos = 0;
    bool wizardSaveChoice = true;

    // AutoTest state variables
    unsigned long autoTestStartTime = 0;
    int autoTestCycle = 0;
    int autoTestPhase = 0;
    uint32_t autoTestOriginalHz = 0;
    bool autoTestOriginalDir = true;
    bool autoTestAborted = false;

    // ---- Admin system state ----
    char  adminPwBuf[ADMIN_PW_MAX_LEN + 2]      = {0}; // New password buffer (setup)
    char  adminConfirmBuf[ADMIN_PW_MAX_LEN + 2]  = {0}; // Confirm buffer (setup)
    char  adminLoginBuf[ADMIN_PW_MAX_LEN + 2]    = {0}; // Login attempt buffer
    int   adminPwPos        = 0;      // Cursor in setup buffers
    int   adminLoginPos     = 0;      // Cursor in login buffer
    bool  adminMismatch     = false;  // Setup PW mismatch flag
    bool  adminLoginFailed  = false;  // Login failure flag
    bool  adminSessionActive = false; // True while admin is authenticated
    State adminLoginReturn  = MENU;   // State to return to after login

    // Pending admin action (what to do after successful login)
    enum AdminAction { ADMIN_ACT_NONE, ADMIN_ACT_DELETE, ADMIN_ACT_PANEL, ADMIN_ACT_MODE };
    AdminAction pendingAdminAction = ADMIN_ACT_NONE;
    int         pendingProfileIdx  = 0;

    // Admin profile panel cursor
    int  adminPanelIndex   = 0;
    bool adminPanelSubMenu = false;

    // User panel cursor
    int  userPanelIndex    = 0;

    // Shared delete-list cursor (used by both admin and user delete lists)
    int  deleteListIndex   = 0;

    // Where to return after wizard completes
    State wizardReturnState = HOME;

    // Where to return from Lang/Tele/Manual/About (the panel that launched them)
    State panelReturnState  = MENU;

    // Confirmation screen state
    ConfirmAction confirmAction  = CONF_DELETE_USER;
    int           confirmProfIdx = -1;
    bool          confirmChoice  = false; // false=NO, true=YES
};
