#pragma once

// ------------------------------ UI String Table ------------------------------
// 'Strings' holds all UI text labels for a given language. The UI consumes this
// as a read-only, zero-terminated C-string table (const char* per label).
// You can provide multiple language instances (e.g., STR_EN, STR_ES) and select
// them at runtime based on user settings.
struct Strings
{
    // ---------------- Home screen ----------------
    const char *hdr_home;     // Header/title on the home screen
    const char *rpm;          // "RPM:" label for tachometer reading
    const char *speed;        // "Speed:" label for clock/frequency
    const char *dir;          // "DIR:" label (direction)
    const char *cw;           // "CW" text (clockwise)
    const char *ccw;          // "CCW" text (counterclockwise)
    const char *brake;        // "BRAKE:" label
    const char *on;           // "ON"
    const char *off;          // "OFF"
    const char *enable;       // "ENABLE:" label
    const char *ld;           // "LD:" label (fault/alarm line)
    const char *alarm;        // "ALARM"
    const char *ok;           // "OK"
    const char *footer_home;  // Footer/help hint displayed on home

    // ---------------- Main menu ------------------
    const char *menu;             // Main menu title
    const char *m_start;          // Start motor entry
    const char *m_stop;           // Stop motor entry
    const char *m_set_ccw;        // Force DIR = CCW action
    const char *m_set_cw;         // Force DIR = CW action
    const char *m_brake_on;       // Turn brake ON
    const char *m_brake_off;      // Turn brake OFF
    const char *m_autotest;       // Run automatic test sequence
    const char *m_select_motor;   // Open motor selection list
    const char *m_add_motor;      // Start add-motor wizard
    const char *m_delete_active;  // Delete currently active profile
    const char *m_settings;       // Open settings submenu
    const char *m_manual;         // Open user manual
    const char *m_about;          // Open about screen
    const char *m_back;           // Go back one level
    const char *footer_menu;      // Footer/help for menu navigation

    // ---------------- Wizard (add/edit profile) ---
    const char *w_name;           // Prompt: motor name
    const char *w_has_brake;      // Prompt: has BRAKE?
    const char *w_has_fg;         // Prompt: has FG?
    const char *w_has_ld;         // Prompt: has LD?
    const char *w_ld_active;      // Prompt: LD polarity
    const char *low;              // Choice: LOW
    const char *high;             // Choice: HIGH
    const char *w_has_stop;       // Prompt: has STOP?
    const char *w_stop_active;    // Prompt: STOP polarity
    const char *w_has_enable;     // Prompt: has ENABLE?
    const char *w_enable_active;  // Prompt: ENABLE polarity
    const char *w_ppr;            // Prompt: pulses per revolution
    const char *w_maxclk;         // Prompt: max clock (Hz)
    const char *w_save;           // Prompt: save profile?
    const char *yes;              // Choice: YES
    const char *no;               // Choice: NO
    const char *hint_yesno;       // Footer hint for yes/no screens
    const char *hint_choice;      // Footer hint for choice lists
    const char *hint_number;      // Footer hint for numeric input
    const char *hint_text;        // Footer hint for text entry

    // ---------------- Settings --------------------
    const char *s_title;          // Settings title
    const char *s_language;       // Language label
    const char *s_lang_en;        // Language option: English
    const char *s_lang_es;        // Language option: Spanish
    const char *s_telemetry;      // Telemetry label
    const char *s_telemetry_on;   // Telemetry ON label
    const char *s_telemetry_off;  // Telemetry OFF label

    // ---------------- About -----------------------
    const char *about_title;      // About title
    const char *about_author;     // Author label
    const char *about_version;    // Version label
    const char *about_build;      // Build label

    // ---------------- Manual ----------------------
    const char *manual_title;     // Manual title
    const char *manual_hint;      // Navigation hint

    // ---------------- Diagnostics -----------------
    const char *diag_title;       // Diagnostics title
    const char *diag_hint;        // Hint to exit diagnostics
};

// English string table (read-only). Keep texts concise to fit 128x64 OLED.
// Tip: Prefer short words to avoid truncation and reduce redraw artifacts.
static const Strings STR_EN = {
    // ---------------- Home ----------------
    "Motor Tester",                                  // hdr_home
    "RPM:",                                          // rpm
    "Speed:",                                        // speed
    "DIR:",                                          // dir
    "CW",                                            // cw
    "CCW",                                           // ccw
    "BRAKE:",                                        // brake
    "ON",                                            // on
    "OFF",                                           // off
    "ENABLE:",                                       // enable
    "LD:",                                           // ld
    "ALARM",                                         // alarm
    "OK",                                            // ok
    "UP/DN:Speed L:Diag R:Menu", // footer_home

    // ---------------- Menu ----------------
    "SETTINGS",                                      // menu
    "Start MOTOR",                                         // m_start
    "Stop MOTOR",                                          // m_stop
    "Set DIR = CCW",                                 // m_set_ccw
    "Set DIR = CW",                                  // m_set_cw
    "Brake ON",                                      // m_brake_on
    "Brake OFF",                                     // m_brake_off
    "Auto Test",                                     // m_autotest
    "Select Motor",                                  // m_select_motor
    "Add Motor",                                     // m_add_motor
    "Delete Active",                                 // m_delete_active
    "Language/Telemetry",                            // m_settings
    "Manual",                                        // m_manual
    "About",                                         // m_about
    "Back",                                          // m_back
    "UP/DN=Move L=Back R=Select",                      // footer_menu

    // ---------------- Wizard ----------------
    "Name:",                                         // w_name
    "Has BRAKE?",                                    // w_has_brake
    "Has FG?",                                       // w_has_fg
    "Has LD?",                                       // w_has_ld
    "LD active when:",                               // w_ld_active
    "LOW",                                           // low
    "HIGH",                                          // high
    "Has STOP?",                                     // w_has_stop
    "STOP active when:",                             // w_stop_active
    "Has ENABLE?",                                   // w_has_enable
    "ENABLE active:",                                // w_enable_active
    "PPR (pulses/rev)",                              // w_ppr
    "Max CLOCK (Hz)",                                // w_maxclk
    "Save profile?",                                 // w_save
    "YES",                                           // yes
    "NO",                                            // no
    "UP/DN=Change L=Back R=Next",                      // hint_yesno
    "UP/DN=Move L=Back R=OK",                          // hint_choice
    "UP/DN=Change L=Back R=OK",                        // hint_number
    "UP/DN=Char L=Back R=Next/END",                    // hint_text

    // ---------------- Settings ----------------
    "SETTINGS",                                      // s_title
    "Language",                                      // s_language
    "English",                                       // s_lang_en
    "Espanol",                                       // s_lang_es
    "Telemetry",                                     // s_telemetry
    "Telemetry: ON",                                 // s_telemetry_on
    "Telemetry: OFF",                                // s_telemetry_off

    // ---------------- About ----------------
    "ABOUT",                                         // about_title
    "Author: Fran-Byte",                             // about_author
    "Version: v2.0",                                 // about_version
    "Build:",                                        // about_build

    // ---------------- Manual ---------------
    "MANUAL",                                        // manual_title
    "UP/DN=Page L=Back",                            // manual_hint

    // ---------------- Diagnostics ------------
    "DIAGNOSTICS",                                   // diag_title
    "LEFT to exit"                                   // diag_hint
};
