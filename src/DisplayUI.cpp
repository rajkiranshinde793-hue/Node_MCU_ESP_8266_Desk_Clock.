#include "DisplayUI.h"

DisplayUI displayUI;

DisplayUI::DisplayUI() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {
    lastUIState = UI_NORMAL;
    lastPomodoroMode = CLOCK;
    lastHighlightedIndex = -1;
    lastAdjustMinutes = -1;
    lastClockSecond = 0;
    lastPomodoroSecond = 0;

    weekDays[0] = "Sun";
    weekDays[1] = "Mon";
    weekDays[2] = "Tue";
    weekDays[3] = "Wed";
    weekDays[4] = "Thu";
    weekDays[5] = "Fri";
    weekDays[6] = "Sat";
}

bool DisplayUI::init() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        return false;
    }
    setMaxBrightness();
    display.setTextColor(SSD1306_WHITE);
    return true;
}

void DisplayUI::setMaxBrightness() {
    uint8_t contrast = 255;
    uint8_t precharge = 0xF1;
    Wire.beginTransmission(0x3C);
    Wire.write(0x00);
    Wire.write(0x81);
    Wire.write(contrast);
    Wire.write(0xD9);
    Wire.write(precharge);
    Wire.endTransmission();
}

void DisplayUI::showStartupMessage(const char* message) {
    display.clearDisplay();
    display.setFont();
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print(message);
    display.display();
}

void DisplayUI::drawNormalClockScreen(NTPClient& timeClient) {
    display.clearDisplay();
    
    // Header: Session Count
    display.setFont();
    display.setTextSize(1);
    display.setCursor(75, 0);
    display.print("Sess: ");
    display.print(pomodoro.getSessionCount());

    // Main 12-hr Clock Time
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(5, 30);
    int rawHours = timeClient.getHours();
    int displayHours = rawHours % 12;
    if (displayHours == 0) displayHours = 12;

    char timeBuffer[16];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02u:%02u:%02u", (unsigned int)displayHours, (unsigned int)timeClient.getMinutes(), (unsigned int)timeClient.getSeconds());
    display.print(timeBuffer);

    // AM/PM Indicator
    display.setFont();
    display.setCursor(85, 20);
    display.print(rawHours >= 12 ? "PM" : "AM");

    display.drawFastHLine(0, 40, 128, SSD1306_WHITE);

    // Day of Week & Date
    time_t rawtime = timeClient.getEpochTime();
    struct tm * ti = localtime(&rawtime);

    display.setFont(&FreeMono9pt7b);
    display.setCursor(5, 56);
    display.print(weekDays[timeClient.getDay()]);

    display.setCursor(70, 56);
    char dateBuffer[16];
    snprintf(dateBuffer, sizeof(dateBuffer), "%02u/%02u", (unsigned int)ti->tm_mday, (unsigned int)(ti->tm_mon + 1));
    display.print(dateBuffer);

    display.display();
}

void DisplayUI::drawNormalPomodoroScreen(NTPClient& timeClient) {
    display.clearDisplay();

    Mode mode = pomodoro.getCurrentMode();
    unsigned long elapsed = pomodoro.getElapsedSeconds();
    unsigned long total = pomodoro.getTotalDurationSeconds();
    unsigned long remaining = pomodoro.getRemainingSeconds();

    int mins = remaining / 60;
    int secs = remaining % 60;

    display.setFont();
    display.setCursor(0, 0);
    display.print(mode == FOCUS ? "FOCUSING" : "BREAK TIME");

    // Right: Current Time (HH:MM)
    int pHours = timeClient.getHours();
    int pDisplayHours = pHours % 12;
    if (pDisplayHours == 0) pDisplayHours = 12;

    char smallTimeBuf[16];
    snprintf(smallTimeBuf, sizeof(smallTimeBuf), "%u:%02u", (unsigned int)pDisplayHours, (unsigned int)timeClient.getMinutes());
    int xPos = 128 - (strlen(smallTimeBuf) * 6);
    display.setCursor(xPos, 0);
    display.print(smallTimeBuf);

    // Main Countdown Timer Font
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(25, 40);
    char timerBuf[16];
    snprintf(timerBuf, sizeof(timerBuf), "%02u:%02u", (unsigned int)(mins > 0 ? mins : 0), (unsigned int)(secs > 0 ? secs : 0));
    display.print(timerBuf);

    // Progress Bar
    int progressWidth = map(elapsed, 0, total, 0, 128);
    display.drawRect(0, 55, 128, 7, SSD1306_WHITE);
    display.fillRect(0, 55, progressWidth, 7, SSD1306_WHITE);

    display.display();
}

void DisplayUI::drawTimerSettingsMenu() {
    int highlighted = getMenuSelection(2); // 0 = FOCUS TIME, 1 = BREAK TIME

    display.clearDisplay();
    display.setFont();
    display.setTextSize(1);

    // Title Header
    display.setCursor(20, 0);
    display.print("TIMER SETTINGS");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    PomodoroSettings st = pomodoro.getSettings();

    // Option 0: FOCUS TIME
    if (highlighted == 0) {
        display.setCursor(5, 20);
        display.print("> FOCUS TIME");
    } else {
        display.setCursor(20, 20);
        display.print("  FOCUS TIME");
    }
    display.setCursor(95, 20);
    display.printf("%dm", st.focusMinutes);

    // Option 1: BREAK TIME
    if (highlighted == 1) {
        display.setCursor(5, 36);
        display.print("> BREAK TIME");
    } else {
        display.setCursor(20, 36);
        display.print("  BREAK TIME");
    }
    display.setCursor(95, 36);
    display.printf("%dm", st.breakMinutes);

    // Footer Hints
    display.drawFastHLine(0, 54, 128, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print("B1:Select   B1x2:Exit");

    display.display();
}

void DisplayUI::drawFocusAdjustmentScreen() {
    int mins = getFocusPresetMinutes();

    display.clearDisplay();
    display.setFont();
    display.setTextSize(1);

    // Header
    display.setCursor(30, 0);
    display.print("FOCUS TIME");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    // Large Value Display
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(30, 38);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d MIN", mins);
    display.print(buf);

    // Footer Hints
    display.setFont();
    display.drawFastHLine(0, 54, 128, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print("B1:Save     B1x2:Exit");

    display.display();
}

void DisplayUI::drawBreakAdjustmentScreen() {
    int mins = getBreakPresetMinutes();

    display.clearDisplay();
    display.setFont();
    display.setTextSize(1);

    // Header
    display.setCursor(30, 0);
    display.print("BREAK TIME");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    // Large Value Display
    display.setFont(&FreeSansBold12pt7b);
    display.setCursor(30, 38);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d MIN", mins);
    display.print(buf);

    // Footer Hints
    display.setFont();
    display.drawFastHLine(0, 54, 128, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.print("B1:Save     B1x2:Exit");

    display.display();
}

void DisplayUI::update(UIState state, NTPClient& timeClient) {
    Mode mode = pomodoro.getCurrentMode();

    if (state == UI_NORMAL) {
        if (mode == CLOCK) {
            unsigned long currentSec = timeClient.getSeconds();
            if (state != lastUIState || mode != lastPomodoroMode || currentSec != lastClockSecond) {
                lastClockSecond = currentSec;
                lastUIState = state;
                lastPomodoroMode = mode;
                drawNormalClockScreen(timeClient);
            }
        } else {
            unsigned long remaining = pomodoro.getRemainingSeconds();
            if (state != lastUIState || mode != lastPomodoroMode || remaining != lastPomodoroSecond) {
                lastPomodoroSecond = remaining;
                lastUIState = state;
                lastPomodoroMode = mode;
                drawNormalPomodoroScreen(timeClient);
            }
        }
    } else if (state == UI_TIMER_SETTINGS) {
        int highlighted = getMenuSelection(2);
        if (state != lastUIState || highlighted != lastHighlightedIndex) {
            lastHighlightedIndex = highlighted;
            lastUIState = state;
            drawTimerSettingsMenu();
        }
    } else if (state == UI_FOCUS_ADJUST) {
        int focusMins = getFocusPresetMinutes();
        if (state != lastUIState || focusMins != lastAdjustMinutes) {
            lastAdjustMinutes = focusMins;
            lastUIState = state;
            drawFocusAdjustmentScreen();
        }
    } else if (state == UI_BREAK_ADJUST) {
        int breakMins = getBreakPresetMinutes();
        if (state != lastUIState || breakMins != lastAdjustMinutes) {
            lastAdjustMinutes = breakMins;
            lastUIState = state;
            drawBreakAdjustmentScreen();
        }
    }
}
