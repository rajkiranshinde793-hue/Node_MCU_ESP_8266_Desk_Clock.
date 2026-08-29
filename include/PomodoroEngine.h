#ifndef POMODORO_ENGINE_H
#define POMODORO_ENGINE_H

#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "GoogleLogger.h"
#include "DNDControl.h"
#include "buzzer.h"

struct PomodoroSettings {
    uint8_t focusMinutes; // Range 25 - 120
    uint8_t breakMinutes; // Range 5 - 30
};

class PomodoroEngine {
public:
    PomodoroEngine();

    void init();
    void update();

    // Controls
    void startFocus();
    void cancelTimer();
    void clearSessions();

    // Settings I/O
    PomodoroSettings getSettings() const;
    void setFocusMinutes(uint8_t mins);
    void setBreakMinutes(uint8_t mins);

    // Status Queries for Display
    Mode getCurrentMode() const;
    uint8_t getSessionCount() const;
    unsigned long getElapsedSeconds() const;
    unsigned long getTotalDurationSeconds() const;
    unsigned long getRemainingSeconds() const;

private:
    Mode currentMode;
    uint8_t sessionCount;
    PomodoroSettings settings;

    unsigned long timerStartTime;
    unsigned long timerDuration;

    void loadEEPROMSettings();
};

extern PomodoroEngine pomodoro;

#endif // POMODORO_ENGINE_H
