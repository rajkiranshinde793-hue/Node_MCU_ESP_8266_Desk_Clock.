#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <Wire.h>
#include "config.h"
#include "PomodoroEngine.h"
#include "pot.h"

// Custom Fonts
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

class DisplayUI {
public:
    DisplayUI();

    bool init();
    void setMaxBrightness();

    // Redraw updater with state-driven optimization
    void update(UIState state, NTPClient& timeClient);

    // Screen Renderers
    void drawNormalClockScreen(NTPClient& timeClient);
    void drawNormalPomodoroScreen(NTPClient& timeClient);
    void drawTimerSettingsMenu();
    void drawFocusAdjustmentScreen();
    void drawBreakAdjustmentScreen();

    // Utility Startup Message
    void showStartupMessage(const char* message);

private:
    Adafruit_SSD1306 display;
    
    UIState lastUIState;
    Mode lastPomodoroMode;
    int lastHighlightedIndex;
    int lastAdjustMinutes;
    unsigned long lastClockSecond;
    unsigned long lastPomodoroSecond;
    String weekDays[7];
};

extern DisplayUI displayUI;

#endif // DISPLAY_UI_H
