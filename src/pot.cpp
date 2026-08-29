#include "pot.h"

static int lastMenuIndex = -1;

void potInit() {
    pinMode(POT_PIN, INPUT);
}

int readPotRaw() {
    return analogRead(POT_PIN);
}

int readPotSmoothed(int samples) {
    if (samples <= 0) samples = 1;
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(POT_PIN);
        delayMicroseconds(100);
    }
    return (int)(sum / samples);
}

int readPotPercent() {
    int raw = readPotSmoothed(8);
    int pct = map(raw, 0, 1023, 0, 100);
    return constrain(pct, 0, 100);
}

int readPotMapped(int minVal, int maxVal) {
    int raw = readPotSmoothed(8);
    int val = map(raw, 0, 1023, minVal, maxVal);
    return constrain(val, min(minVal, maxVal), max(minVal, maxVal));
}

int getMenuSelection(int itemCount) {
    if (itemCount <= 0) return 0;
    int raw = readPotSmoothed(8);
    int newIndex = map(raw, 0, 1023, 0, itemCount);
    if (newIndex >= itemCount) newIndex = itemCount - 1;
    if (newIndex < 0) newIndex = 0;

    if (lastMenuIndex == -1) {
        lastMenuIndex = newIndex;
    } else if (abs(newIndex - lastMenuIndex) >= 1) {
        int stepWidth = 1023 / itemCount;
        int currentCenter = (lastMenuIndex * stepWidth) + (stepWidth / 2);
        if (abs(raw - currentCenter) > (stepWidth / 2 + 15)) {
            lastMenuIndex = newIndex;
        }
    }
    return lastMenuIndex;
}

int getFocusPresetMinutes() {
    // 25 to 120 minutes in 5-minute increments (20 steps: 25, 30, 35, ..., 120)
    int stepIndex = getMenuSelection(20);
    int minutes = 25 + (stepIndex * 5);
    return constrain(minutes, 25, 120);
}

int getBreakPresetMinutes() {
    // 5 to 30 minutes in 5-minute increments (6 steps: 5, 10, 15, 20, 25, 30)
    int stepIndex = getMenuSelection(6);
    int minutes = 5 + (stepIndex * 5);
    return constrain(minutes, 5, 30);
}
