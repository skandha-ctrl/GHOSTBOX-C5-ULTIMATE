#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include <Arduino.h>

// ═══════════════════════════════════════════════════════════════════════════
// VIRTUAL KEYBOARD · 10 columns × 4 alphanumeric rows + 5 special keys
// · Navigation: UP/DOWN column by column (vertical first)
// · SHIFT toggle uppercase
// · Standard QWERTY layout
// · Returns input string or "" if canceled
// ═══════════════════════════════════════════════════════════════════════════

// Displays the keyboard and returns the string entered by the user.
// If the user cancels (X), returns an empty string.
//
// Parameters:
// title: title displayed at top (e.g. "WIFI PASSWORD")
// subtitle: line under title (e.g. "Net: Home_WiFi")
// maxLen: maximum characters allowed (62 default)
// maskInput: true = show asterisks (***), false = plain text
//
// Returns:
// Entered string, or "" if canceled
String virtualKeyboardInput(const String& title,
                             const String& subtitle,
                             int maxLen = 62,
                             bool maskInput = false);

#endif