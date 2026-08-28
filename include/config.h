#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Hardware Pins 
#define POT_PIN A0
#define BUTTON_1_PIN D5
#define BUTTON_PIN BUTTON_1_PIN // Alias for backward compatibility
#define BUTTON_2_PIN D3
#define BUTTON_3_PIN D4
#define BUTTON_4_PIN D6
#define BUZZER_PIN D7

// OLED display settings 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Constants 
#define EEPROM_ADDR 0        
#define MAX_SESSIONS 24      
#define LONG_PRESS_MS 500
#define SYNC_INTERVAL 1800000 // 30 Minutes

// NTP Settings 
#define NTP_OFFSET 19800 // GMT +5:30

// Enums (Must be here so main.cpp understands them) 
enum Mode { CLOCK, FOCUS, BREAK };
enum NetworkState { NET_IDLE, NET_CONNECTING, NET_UPDATING };

#endif