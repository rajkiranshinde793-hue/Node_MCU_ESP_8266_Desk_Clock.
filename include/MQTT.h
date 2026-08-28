#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"

// Initialize MQTT client configuration
void initMQTT();

// Maintain MQTT connection and process incoming/outgoing packets
void handleMQTT();

// Publish CHIRAG logical button event (1 -> "chirag_button_1", 2 -> "chirag_button_2", 3 -> "chirag_button_3")
bool sendChiragButtonEvent(int buttonNumber);

// Check if MQTT is currently connected
bool isMQTTConnected();

#endif // MQTT_H
