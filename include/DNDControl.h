#ifndef DND_CONTROL_H
#define DND_CONTROL_H

#include <Arduino.h>

// Request a DND state change
void queueDNDChange(bool enable);

// Check if a change request is pending
bool dndHasPending();

// Perform the Webhook (Assumes WiFi is ALREADY Connected)
bool performDndSync();

#endif