#include "SoundUtils.h"
#include <Arduino.h>
#include "Settings.h"
#include "Pins.h"

void initSound() {
#if defined(BUZZER_PIN) && (BUZZER_PIN >= 0)
    if (digitalPinIsValid(BUZZER_PIN)) {
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
    }
#endif
}

void beep(int freq, int duration) {
#if !defined(BUZZER_PIN) || (BUZZER_PIN < 0)
    (void)freq;
    (void)duration;
    return;
#else
    if (BUZZER_PIN < 0 || !digitalPinIsValid(BUZZER_PIN)) return;
    if (!soundEnabled || freq <= 0) return;

    ledcAttach(BUZZER_PIN, 2000, 8);
    ledcWriteTone(BUZZER_PIN, freq);
    delay(duration);
    ledcWriteTone(BUZZER_PIN, 0);
    ledcDetach(BUZZER_PIN);
#endif
}

void clickTone() {
    beep(1800, 20);
}

void successTone() {
    beep(2000, 40);
    delay(30);
    beep(2600, 60);
}

void errorTone() {
    beep(800, 80);
    delay(40);
    beep(600, 100);
}

void alertTone() {
    beep(2800, 80);
    delay(30);
    beep(2400, 100);
}

