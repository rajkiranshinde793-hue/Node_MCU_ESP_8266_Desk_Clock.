#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Hardware Pins 
#define BUTTON_PIN D5
#define BUZZER_PIN D6

// OLED display settings 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Constants 
#define EEPROM_ADDR 0        
#define MAX_SESSIONS 24      
#define LONG_PRESS_MS 500
#define SYNC_INTERVAL 1800000 // 30 Minutes

// Network Timeouts
#define WIFI_CONNECT_TIMEOUT_MS 10000 
#define HTTP_TIMEOUT_MS 5000

// NTP Settings 
#define NTP_OFFSET 19800 // GMT +5:30

// Enums 
enum Mode { CLOCK, FOCUS, BREAK };

// --- NEW SYSTEM STATE MACHINE ---
enum SystemState { 
    SYS_IDLE,           // Radio OFF, CPU Light Sleep capable
    SYS_WIFI_START,     // Wake Radio, Begin Connection
    SYS_WIFI_WAIT,      // Wait for IP
    SYS_TASK_RUN,       // Perform HTTP uploads/NTP
    SYS_WIFI_STOP       // Disconnect, Radio Sleep
};

#endif