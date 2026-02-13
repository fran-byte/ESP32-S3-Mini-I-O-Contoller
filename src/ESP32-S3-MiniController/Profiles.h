#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

// ------------------------------ MotorProfile ------------------------------
// Describes a motor profile: capabilities (brake, FG, LD, stop, enable),
// signal polarities, tachometer PPR, and a safety cap for the clock (Hz).
// The name is a short, human-readable label stored alongside.
struct MotorProfile {
  char   name[20];
  bool   hasBrake;
  bool   hasFG;
  bool   hasLD;
  bool   ldActiveLow;      // true if LD is asserted when the input reads LOW
  bool   hasStop;
  bool   stopActiveHigh;   // true if STOP output is asserted HIGH
  bool   hasEnable;
  bool   enableActiveHigh; // true if ENABLE output is asserted HIGH
  uint8_t  ppr;            // Pulses per revolution (tachometer/FG)
  uint32_t maxClockHz;     // Safety limit for the generated clock

  // Initialize with safe, generic defaults.
  void setDefaults() {
    strncpy(name, "Unnamed", sizeof(name));
    hasBrake = false;
    hasFG = false;
    hasLD = false;
    ldActiveLow = true;
    hasStop = false;
    stopActiveHigh = true;
    hasEnable = false;
    enableActiveHigh = true;
    ppr = 6;
    maxClockHz = 20000;
  }
};

// ------------------------------ ProfileStore ------------------------------
// Persistent storage for motor profiles using ESP32 Preferences (NVS).
// Layout:
//   - "count"  : number of profiles stored (0..MAX_PROFILES)
//   - "active" : active profile index (0..count-1) or 255 if none
//   Per-profile keys (for index i):
//     "mi_name", "mi_br", "mi_fg", "mi_ld", "mi_lda",
//     "mi_st", "mi_sta", "mi_en", "mi_ena", "mi_ppr", "mi_max"
class ProfileStore {
public:
  // Open the NVS namespace and read the number of profiles and active index.
  // If a stored count is invalid (>MAX_PROFILES), reset to 0 for safety.
  void begin() {
    prefs.begin("motors", false);
    count = prefs.getUChar("count", 0);
    activeIndex = prefs.getUChar("active", 0);
    if (count > MAX_PROFILES) count = 0;
  }

  int getCount() const { return count; }
  int getActiveIndex() const { return activeIndex; }

  // Load a profile at index 'idx' into 'm'.
  // Returns false if the index is out of range.
  bool load(int idx, MotorProfile &m) {
    if (idx < 0 || idx >= count) return false;

    char key[16];

    // Name (string) and all boolean/numeric fields.
    snprintf(key, sizeof(key), "m%d_name", idx);
    String s = prefs.getString(key, "Unnamed");
    strncpy(m.name, s.c_str(), sizeof(m.name));

    snprintf(key, sizeof(key), "m%d_br", idx);   m.hasBrake         = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_fg", idx);   m.hasFG            = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_ld", idx);   m.hasLD            = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_lda", idx);  m.ldActiveLow      = prefs.getBool(key, true);
    snprintf(key, sizeof(key), "m%d_st", idx);   m.hasStop          = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_sta", idx);  m.stopActiveHigh   = prefs.getBool(key, true);
    snprintf(key, sizeof(key), "m%d_en", idx);   m.hasEnable        = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_ena", idx);  m.enableActiveHigh = prefs.getBool(key, true);
    snprintf(key, sizeof(key), "m%d_ppr", idx);  m.ppr              = prefs.getUChar(key, 6);
    snprintf(key, sizeof(key), "m%d_max", idx);  m.maxClockHz       = prefs.getUInt(key, 20000);

    return true;
  }

  // Save 'm' into slot 'idx' (0..MAX_PROFILES-1). Extends the stored count
  // if we save into a new, next-free slot. Returns false if idx is invalid.
  bool save(int idx, const MotorProfile &m) {
    if (idx < 0 || idx >= MAX_PROFILES) return false;

    char key[16];

    snprintf(key, sizeof(key), "m%d_name", idx); prefs.putString(key, m.name);
    snprintf(key, sizeof(key), "m%d_br",   idx); prefs.putBool  (key, m.hasBrake);
    snprintf(key, sizeof(key), "m%d_fg",   idx); prefs.putBool  (key, m.hasFG);
    snprintf(key, sizeof(key), "m%d_ld",   idx); prefs.putBool  (key, m.hasLD);
    snprintf(key, sizeof(key), "m%d_lda",  idx); prefs.putBool  (key, m.ldActiveLow);
    snprintf(key, sizeof(key), "m%d_st",   idx); prefs.putBool  (key, m.hasStop);
    snprintf(key, sizeof(key), "m%d_sta",  idx); prefs.putBool  (key, m.stopActiveHigh);
    snprintf(key, sizeof(key), "m%d_en",   idx); prefs.putBool  (key, m.hasEnable);
    snprintf(key, sizeof(key), "m%d_ena",  idx); prefs.putBool  (key, m.enableActiveHigh);
    snprintf(key, sizeof(key), "m%d_ppr",  idx); prefs.putUChar (key, m.ppr);
    snprintf(key, sizeof(key), "m%d_max",  idx); prefs.putUInt  (key, m.maxClockHz);

    // If saving beyond current count, grow count and persist it.
    if (idx >= count) {
      count = idx + 1;
      prefs.putUChar("count", count);
    }
    return true;
  }

  // Append a new profile at the end (if capacity allows).
  bool append(const MotorProfile &m) {
    if (count >= MAX_PROFILES) return false;
    return save(count, m);
  }

  // Remove profile at 'idx' by shifting subsequent entries left,
  // clearing the last slot keys, and updating count/active index.
  void remove(int idx) {
    if (idx < 0 || idx >= count) return;

    MotorProfile tmp;
    for (int i = idx; i < count - 1; ++i) {
      load(i + 1, tmp);
      save(i, tmp);
    }

    // Clear the tail keys for the last, now-unused slot.
    char key[16];
    int last = count - 1;
    const char* sfx[] = { "name","br","fg","ld","lda","st","sta","en","ena","ppr","max" };
    for (auto s : sfx) {
      snprintf(key, sizeof(key), "m%d_%s", last, s);
      prefs.remove(key);
    }

    // Update count and persist.
    count--;
    prefs.putUChar("count", count);

    // If active index is now out of range, reset it to 0 (or 255 to signal none).
    if (activeIndex >= count) {
      activeIndex = (count > 0 ? 0 : 255);
      prefs.putUChar("active", activeIndex);
    }
  }

  // Load the active profile; returns false if none is available.
  bool loadActive(MotorProfile &m) {
    if (count == 0 || activeIndex >= count) return false;
    return load(activeIndex, m);
  }

  // Mark a profile as active and persist the index.
  void setActive(int idx) {
    if (idx >= 0 && idx < count) {
      activeIndex = idx;
      prefs.putUChar("active", activeIndex);
    }
  }

  // Retrieve the stored name of a profile by index (or "-" if invalid).
  String nameOf(int idx) {
    if (idx < 0 || idx >= count) return String("-");
    char key[16];
    snprintf(key, sizeof(key), "m%d_name", idx);
    return prefs.getString(key, "Unnamed");
  }

private:
  Preferences prefs;
  uint8_t count = 0;
  uint8_t activeIndex = 0;  // 255 can be used to denote "no active" when count==0
}
;
