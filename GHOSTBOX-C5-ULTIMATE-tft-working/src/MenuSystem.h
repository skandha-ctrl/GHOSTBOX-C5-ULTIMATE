#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#include <Arduino.h>
#include "Icons.h"

// ═══════════════════════════════════════════════════════════════════════════
// MENU SYSTEM · Master carousel + hierarchical submenus
//
// Usage from main.cpp:
// runMainMenu(); // infinite loop, never returns
//
// Usage from tools requiring custom menus/lists:
// int sel = runSubMenu("WIFI TOOLS", items, 4);
// if (sel == -1) return; // user selected BACK
// switch (sel) { ... }
// ═══════════════════════════════════════════════════════════════════════════

// ── Master carousel entry ────────────────────────────────────────
struct MainMenuEntry {
    const char* title;           // Category name (e.g. "WIFI TOOLS")
    const char* subtitle;        // Short description (e.g. "Scan, Deauth, ...")
    IconID      icon;            // Icon ID to display
    void        (*handler)();    // Function called on OK press
};

// ── Public API ───────────────────────────────────────────────────────────

// Main carousel. Call from loop() or setup(); does not return.
void runMainMenu();

// Vertical scrolling list submenu. Returns:
// · -1 if the user selected BACK or held OK
// · index 0..count-1 of the selected item
int  runSubMenu(const char* title, const char* items[], int count);

#endif