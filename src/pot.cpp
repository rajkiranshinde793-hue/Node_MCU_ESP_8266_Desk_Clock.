#include "pot.h"

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
