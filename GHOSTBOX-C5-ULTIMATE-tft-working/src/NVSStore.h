#ifndef NVS_STORE_H
#define NVS_STORE_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// NVSStore · Simple wrapper around ESP32 Preferences (NVS)
// · Persistent storage in flash: survives reboots and power losses
// · Keys limited to 15 characters (NVS limit)
// · Clean API: get with default, set, erase
// ═══════════════════════════════════════════════════════════════════════════

// Initialize the NVS namespace. Call ONCE from setup().
void nvsBegin();

// Close NVS storage (optional, at program termination)
void nvsEnd();

// ── Read ──────────────────────────────────────────────────────────────────
// Returns the saved value, or `defaultValue` if the key does not exist.
bool          nvsGetBool(const char* key, bool defaultValue);
int           nvsGetInt(const char* key, int defaultValue);
unsigned long nvsGetULong(const char* key, unsigned long defaultValue);
String        nvsGetString(const char* key, const String& defaultValue);

// ── Write ─────────────────────────────────────────────────────────────────
// Saves immediately to flash.
void nvsSetBool(const char* key, bool value);
void nvsSetInt(const char* key, int value);
void nvsSetULong(const char* key, unsigned long value);
void nvsSetString(const char* key, const String& value);

// ── Utilities ─────────────────────────────────────────────────────────────
void nvsErase(const char* key);      // Erases a specific key
void nvsEraseAll();                  // ⚠️ Erases all stored preferences

#endif