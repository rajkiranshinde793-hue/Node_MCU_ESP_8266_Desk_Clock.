#include "PomodoroEngine.h"

PomodoroEngine pomodoro;

PomodoroEngine::PomodoroEngine() {
    currentMode = CLOCK;
    sessionCount = 0;
    settings.focusMinutes = 25;
    settings.breakMinutes = 5;
    timerStartTime = 0;
    timerDuration = 0;
}

void PomodoroEngine::loadEEPROMSettings() {
    sessionCount = EEPROM.read(EEPROM_ADDR_SESSIONS);
    if (sessionCount > MAX_SESSIONS) {
        sessionCount = 0;
        EEPROM.write(EEPROM_ADDR_SESSIONS, 0);
    }

    settings.focusMinutes = EEPROM.read(EEPROM_ADDR_FOCUS);
    if (settings.focusMinutes < 5 || settings.focusMinutes > 120) {
        settings.focusMinutes = 25;
        EEPROM.write(EEPROM_ADDR_FOCUS, 25);
    }

    settings.breakMinutes = EEPROM.read(EEPROM_ADDR_BREAK);
    if (settings.breakMinutes < 1 || settings.breakMinutes > 60) {
        settings.breakMinutes = 5;
        EEPROM.write(EEPROM_ADDR_BREAK, 5);
    }

    EEPROM.commit();
}

void PomodoroEngine::init() {
    EEPROM.begin(4);
    loadEEPROMSettings();
}

void PomodoroEngine::startFocus() {
    if (currentMode == CLOCK) {
        currentMode = FOCUS;
        timerDuration = (unsigned long)settings.focusMinutes * 60;
        timerStartTime = millis();
        playFocusStartTune();
        queueDNDChange(true);
    }
}

void PomodoroEngine::cancelTimer() {
    currentMode = CLOCK;
    timerStartTime = 0;
    timerDuration = 0;
    playResetTune();
    queueDNDChange(false);
}

void PomodoroEngine::clearSessions() {
    sessionCount = 0;
    EEPROM.write(EEPROM_ADDR_SESSIONS, 0);
    EEPROM.commit();
    playResetTune();
}

PomodoroSettings PomodoroEngine::getSettings() const {
    return settings;
}

void PomodoroEngine::setFocusMinutes(uint8_t mins) {
    if (mins < 5) mins = 5;
    if (mins > 120) mins = 120;
    settings.focusMinutes = mins;
    EEPROM.write(EEPROM_ADDR_FOCUS, settings.focusMinutes);
    EEPROM.commit();
    playSyncSuccessTune();
}

void PomodoroEngine::setBreakMinutes(uint8_t mins) {
    if (mins < 1) mins = 1;
    if (mins > 60) mins = 60;
    settings.breakMinutes = mins;
    EEPROM.write(EEPROM_ADDR_BREAK, settings.breakMinutes);
    EEPROM.commit();
    playSyncSuccessTune();
}

Mode PomodoroEngine::getCurrentMode() const {
    return currentMode;
}

uint8_t PomodoroEngine::getSessionCount() const {
    return sessionCount;
}

unsigned long PomodoroEngine::getElapsedSeconds() const {
    if (currentMode == CLOCK || timerStartTime == 0) return 0;
    return (millis() - timerStartTime) / 1000;
}

unsigned long PomodoroEngine::getTotalDurationSeconds() const {
    return timerDuration;
}

unsigned long PomodoroEngine::getRemainingSeconds() const {
    unsigned long elapsed = getElapsedSeconds();
    if (elapsed >= timerDuration) return 0;
    return timerDuration - elapsed;
}

void PomodoroEngine::update() {
    if (currentMode == CLOCK) return;

    unsigned long elapsed = getElapsedSeconds();
    if (elapsed >= timerDuration) {
        if (currentMode == FOCUS) {
            logToGoogle("Focus", settings.focusMinutes);
            queueDNDChange(false);

            currentMode = BREAK;
            timerDuration = (unsigned long)settings.breakMinutes * 60;
            timerStartTime = millis();
            playBreakStartTune();
        } else if (currentMode == BREAK) {
            sessionCount++;
            if (sessionCount > MAX_SESSIONS) sessionCount = 0;
            EEPROM.write(EEPROM_ADDR_SESSIONS, sessionCount);
            EEPROM.commit();

            currentMode = CLOCK;
            timerStartTime = 0;
            timerDuration = 0;
            playSessionCompleteTune();
        }
    }
}
