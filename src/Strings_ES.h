#pragma once
#include "Strings_EN.h"

// ------------------------------ Spanish String Table ------------------------------
// Spanish translation of the complete UI string set. This mirrors STR_EN exactly,
// field-by-field, so the UI can switch languages simply by selecting a different
// 'Strings' instance at runtime. Keep texts short enough to fit on a 128x64 OLED.

static const Strings STR_ES = {
    // ---------------- Home ----------------
    "Tester de Motor",                               // hdr_home
    "RPM:",                                          // rpm
    "Velocidad:",                                    // speed
    "DIR:",                                          // dir
    "CW",                                            // cw
    "CCW",                                           // ccw
    "FRENO:",                                        // brake
    "ON",                                            // on
    "OFF",                                           // off
    "ENABLE:",                                       // enable
    "LD:",                                           // ld
    "ALARM",                                         // alarm
    "OK",                                            // ok
    "UP/DOWN: Vel.  SEL: Menu  MANTEN SEL: Atras/Arr-Det", // footer_home

    // ---------------- Menu ----------------
    "CONFIGURACION",                                 // menu
    "Arrancar",                                      // m_start
    "Parar",                                         // m_stop
    "DIR = CCW",                                     // m_set_ccw
    "DIR = CW",                                      // m_set_cw
    "Freno ON",                                      // m_brake_on
    "Freno OFF",                                     // m_brake_off
    "Seleccionar Motor",                             // m_select_motor
    "Anadir Motor",                                  // m_add_motor
    "Borrar Activo",                                 // m_delete_active
    "Lenguaje/Telemetria",                           // m_settings
    "Acerca de",                                     // m_about
    "Atras",                                         // m_back
    "UP/DOWN=Mover  SEL=Seleccionar",                // footer_menu

    // ---------------- Wizard ----------------
    "Nombre:",                                       // w_name
    "Tiene FRENO?",                                  // w_has_brake
    "Tiene FG?",                                     // w_has_fg
    "Tiene LD?",                                     // w_has_ld
    "LD activo cuando:",                             // w_ld_active
    "LOW",                                           // low
    "HIGH",                                          // high
    "Tiene STOP?",                                   // w_has_stop
    "STOP activo cuando:",                           // w_stop_active
    "Tiene ENABLE?",                                 // w_has_enable
    "ENABLE activo:",                                // w_enable_active
    "PPR (pulsos/vuelta)",                           // w_ppr
    "CLOCK max (Hz)",                                // w_maxclk
    "Guardar perfil?",                               // w_save
    "SI",                                            // yes
    "NO",                                            // no
    "UP/DOWN=Cambiar  SEL=Siguiente",                // hint_yesno
    "UP/DOWN=Mover  SEL=OK",                         // hint_choice
    "UP/DOWN=Cambiar  SEL=OK",                       // hint_number
    "UP/DN=Car SEL=Sig/END",                         // hint_text

    // ---------------- Settings ----------------
    "CONFIGURACION",                                 // s_title
    "Idioma",                                        // s_language
    "English",                                       // s_lang_en
    "Espanol",                                       // s_lang_es
    "Telemetria",                                    // s_telemetry
    "Telemetria: ON",                                // s_telemetry_on
    "Telemetria: OFF",                               // s_telemetry_off

    // ---------------- About ----------------
    "ACERCA DE",                                     // about_title
    "Autor: Fran-Byte",                              // about_author
    "Version: v2.0",                                 // about_version
    "Build:",                                        // about_build

    // ---------------- Diagnostics ----------------
    "DIAGNOSTICO",                                   // diag_title
    "MANTEN SEL para salir"                          // diag_hint
};
