#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>
#include "config.h"

// Initialize buzzer pin
void buzzerInit();

// Core tone trigger function
void triggerBuzzer(int beeps, int durationMs = 200, int frequency = 1000);

// Specialized feedback tunes/tones for project events
void playButtonTone();
void playFocusStartTune();
void playBreakStartTune();
void playSessionCompleteTune();
void playResetTune();
void playErrorTone();
void playSyncSuccessTune();

#endif // BUZZER_H
