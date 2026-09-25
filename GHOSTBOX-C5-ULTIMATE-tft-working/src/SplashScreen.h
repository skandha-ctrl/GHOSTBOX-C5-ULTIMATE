#ifndef SPLASH_SCREEN_H

#define SPLASH_SCREEN_H



#include <Arduino.h>



// ═══════════════════════════════════════════════════════════════════════════
// SPLASH SCREEN · Device boot welcome screen
//
// Features:
// · AXOLOTL pixel-art with glasses (project mascot)
// · Title "GHOSTBOX C5" with type-on animation
// · System and hardware audit info
// · Sequence of boot progress steps with ascending audio beeps
// · Flashing "PRESS OK" prompt
// · Waits until user presses OK to continue
// ═══════════════════════════════════════════════════════════════════════════



void runSplashScreen();



#endif