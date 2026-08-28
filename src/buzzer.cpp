#include "buzzer.h"

void buzzerInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void triggerBuzzer(int beeps, int durationMs, int frequency) {
    // Clamp frequency between 700 and 1200 Hz for optimal passive buzzer loudness
    if (frequency < 700) frequency = 700;
    if (frequency > 1200) frequency = 1200;

    for (int i = 0; i < beeps; i++) {
        tone(BUZZER_PIN, frequency);
        delay(durationMs);
        noTone(BUZZER_PIN);
        if (i < beeps - 1) {
            delay(100);
        }
    }
}

void playButtonTone() {
    tone(BUZZER_PIN, 1050);
    delay(40);
    noTone(BUZZER_PIN);
}

void playFocusStartTune() {
    tone(BUZZER_PIN, 800);
    delay(100);
    tone(BUZZER_PIN, 1000);
    delay(100);
    tone(BUZZER_PIN, 1200);
    delay(150);
    noTone(BUZZER_PIN);
}

void playBreakStartTune() {
    tone(BUZZER_PIN, 750);
    delay(120);
    tone(BUZZER_PIN, 950);
    delay(120);
    tone(BUZZER_PIN, 1150);
    delay(200);
    noTone(BUZZER_PIN);
}

void playSessionCompleteTune() {
    for (int i = 0; i < 3; i++) {
        tone(BUZZER_PIN, 1100);
        delay(100);
        noTone(BUZZER_PIN);
        delay(80);
    }
}

void playResetTune() {
    tone(BUZZER_PIN, 1150);
    delay(200);
    tone(BUZZER_PIN, 750);
    delay(350);
    noTone(BUZZER_PIN);
}

void playErrorTone() {
    tone(BUZZER_PIN, 700);
    delay(200);
    noTone(BUZZER_PIN);
    delay(100);
    tone(BUZZER_PIN, 700);
    delay(200);
    noTone(BUZZER_PIN);
}

void playSyncSuccessTune() {
    tone(BUZZER_PIN, 900);
    delay(80);
    tone(BUZZER_PIN, 1200);
    delay(120);
    noTone(BUZZER_PIN);
}
