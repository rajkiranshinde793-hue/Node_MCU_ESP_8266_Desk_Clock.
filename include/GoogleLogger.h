#pragma once
#ifndef GOOGLE_LOGGER_H
#define GOOGLE_LOGGER_H

#include <Arduino.h>

// Queue a log entry in RAM
void queueLog(String type, int durationMinutes);

// Check if data is waiting to be uploaded
bool loggerHasPending();

// Perform the upload (Assumes WiFi is ALREADY Connected)
// Returns true if successful, false if failed
bool performLogUpload();

#endif