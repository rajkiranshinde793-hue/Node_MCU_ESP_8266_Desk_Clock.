#ifndef POT_H
#define POT_H

#include <Arduino.h>
#include "config.h"

// Initialize potentiometer pin
void potInit();

// Read raw 10-bit analog value (0 - 1023)
int readPotRaw();

// Read smoothed analog value to reduce 10K pot noise
int readPotSmoothed(int samples = 8);

// Read potentiometer value as a percentage (0 - 100%)
int readPotPercent();

// Map raw potentiometer reading to a custom target range [minVal, maxVal]
int readPotMapped(int minVal, int maxVal);

// Get menu selection index (0 to itemCount - 1) with hysteresis filtering
int getMenuSelection(int itemCount);

// Get focus preset minutes (25 to 120 in 5-minute increments) with hysteresis filtering
int getFocusPresetMinutes();

// Get break preset minutes (5 to 30 in 5-minute increments) with hysteresis filtering
int getBreakPresetMinutes();

#endif // POT_H
