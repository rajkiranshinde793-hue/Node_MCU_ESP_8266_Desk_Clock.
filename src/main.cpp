#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h> 

#include "Config.h"
#include "credentials.h"  
#include "GoogleLogger.h"
#include "DNDControl.h"

// Custom Fonts 
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

// --- OBJECT INITIALIZATION ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", NTP_OFFSET);

// --- VARIABLES ---
String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}; 

// Logic States
Mode currentMode = CLOCK;
SystemState sysState = SYS_IDLE;

// Timers
unsigned long timerStartTime = 0;
unsigned long timerDuration = 0; 
unsigned long lastSyncTime = 0;
unsigned long wifiConnectStart = 0;

// Button Debounce
bool lastButtonState = HIGH;
unsigned long buttonPressedTime = 0;

// Session Data
int sessionCount = 0;

// --- FUNCTION PROTOTYPES ---
void setBrightness();
void systemStateMachine();
void triggerBuzzer(int beeps, int duration = 200);
void saveSession();
void clearSessions();
void handleButton();
void drawClock();
void drawPomodoro();

// --- HELPER FUNCTIONS ---
void setBrightness() {
  Wire.beginTransmission(0x3C);
  Wire.write(0x00); Wire.write(0x81); Wire.write(100); 
  Wire.write(0xD9); Wire.write(0xF1);
  Wire.endTransmission();
}

void setup() {
  system_update_cpu_freq(80);
  Serial.begin(9600);
  
  // 1. HARD DISABLE WIFI ON BOOT
  WiFi.mode(WIFI_OFF); 
  WiFi.forceSleepBegin();
  delay(1); 
  
  EEPROM.begin(4); 
  sessionCount = EEPROM.read(EEPROM_ADDR);
  if (sessionCount > MAX_SESSIONS) sessionCount = 0;

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  
  display.clearDisplay();
  setBrightness();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 30);
  display.print("SYSTEM BOOT");
  display.display();
  delay(500);

  // Force an initial sync by setting time to 0
  lastSyncTime = 0; 
}

// --- CORE SYSTEM STATE MACHINE ---
void systemStateMachine() {
  unsigned long now = millis();

  switch (sysState) {
    // ----------------------------------------------------
    // STATE 1: IDLE (Monitoring)
    // ----------------------------------------------------
    case SYS_IDLE:
      {
        bool needsNTP = (now - lastSyncTime > SYNC_INTERVAL) || (lastSyncTime == 0);
        bool needsLog = loggerHasPending();
        bool needsDND = dndHasPending();

        if (needsNTP || needsLog || needsDND) {
          sysState = SYS_WIFI_START;
        }
      }
      break;

    // ----------------------------------------------------
    // STATE 2: WAKE RADIO
    // ----------------------------------------------------
    case SYS_WIFI_START:
      Serial.println("[SYS] Waking WiFi...");
      WiFi.forceSleepWake();
      delay(1);
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      wifiConnectStart = now;
      sysState = SYS_WIFI_WAIT;
      break;

    // ----------------------------------------------------
    // STATE 3: WAIT FOR CONNECTION
    // ----------------------------------------------------
    case SYS_WIFI_WAIT:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[SYS] WiFi Connected: ");
        Serial.println(WiFi.localIP());
        sysState = SYS_TASK_RUN;
      } 
      else if (now - wifiConnectStart > WIFI_CONNECT_TIMEOUT_MS) {
        Serial.println("[SYS] WiFi Timeout. Aborting.");
        sysState = SYS_WIFI_STOP; // Fail safely
      }
      break;

    // ----------------------------------------------------
    // STATE 4: RUN NETWORK TASKS
    // ----------------------------------------------------
    case SYS_TASK_RUN:
      {
        // 1. NTP Sync
        if ((now - lastSyncTime > SYNC_INTERVAL) || lastSyncTime == 0) {
           timeClient.begin();
           if(timeClient.forceUpdate()) {
              lastSyncTime = now;
              Serial.println("[SYS] Time Synced");
           }
        }

        // 2. DND Webhook
        if (dndHasPending()) {
            performDndSync();
        }

        // 3. Google Logging
        if (loggerHasPending()) {
            performLogUpload();
        }

        // Task complete
        sysState = SYS_WIFI_STOP;
      }
      break;

    // ----------------------------------------------------
    // STATE 5: POWER DOWN
    // ----------------------------------------------------
    case SYS_WIFI_STOP:
      Serial.println("[SYS] Sleeping Radio...");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin(); // Critical for Light Sleep
      delay(1); 
      sysState = SYS_IDLE;
      break;
  }
}

void loop() {
  handleButton();
  systemStateMachine();

  if (currentMode == CLOCK) {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 1000) {
       lastUpdate = millis();
       display.clearDisplay();
       drawClock();
       display.display();
    }
    
    // LIGHT SLEEP MANAGEMENT
    // Only sleep if WiFi is completely OFF (IDLE state)
    if (sysState == SYS_IDLE) {
       delay(50); // ESP8266 Auto-Light-Sleep engages here
    } else {
       delay(1); // Minimal delay to keep Watchdog happy while networking
    }

  } else {
    // Focus Mode - High refresh rate
    display.clearDisplay();
    drawPomodoro();
    display.display();
    delay(10); 
  }
}

// --- LOGIC HANDLERS ---

void handleButton() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressedTime = now;
  }
  
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    unsigned long pressDuration = now - buttonPressedTime;
    
    // LONG PRESS: Reset / Mode Switch
    if (pressDuration >= LONG_PRESS_MS) {
      if (currentMode == CLOCK) {
        clearSessions();
      } else {
        // Cancel Focus
        currentMode = CLOCK;
        queueDNDChange(false); // Queue DND OFF
        triggerBuzzer(2, 100);
      }
    } 
    // SHORT PRESS: Start Focus
    else if (pressDuration > 50) {
      if (currentMode == CLOCK) {
        currentMode = FOCUS;
        timerDuration = 25 * 60; 
        timerStartTime = now;
        queueDNDChange(true); // Queue DND ON
        triggerBuzzer(1); 
      }
    }
  }
  lastButtonState = currentButtonState;
}

void triggerBuzzer(int beeps, int duration) {
  for(int i = 0; i < beeps; i++) {
    tone(BUZZER_PIN, 1000);
    delay(duration);
    noTone(BUZZER_PIN);
    delay(100);
  }
}

void saveSession() {
  sessionCount++;
  if (sessionCount > MAX_SESSIONS) sessionCount = 0;
  EEPROM.write(EEPROM_ADDR, sessionCount);
  EEPROM.commit();
}

void clearSessions() {
  sessionCount = 0;
  EEPROM.write(EEPROM_ADDR, 0);
  EEPROM.commit();
  triggerBuzzer(1, 1000);
}

void drawClock() {
  display.setFont();
  display.setTextSize(1);
  display.setCursor(75, 0);
  display.print("Sess: "); display.print(sessionCount);

  // Status Indicator (Dot at top left if WiFi is active)
  if (sysState != SYS_IDLE) display.fillCircle(2, 2, 2, SSD1306_WHITE);

  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(5, 30); 
  
  int rawHours = timeClient.getHours();
  int displayHours = rawHours % 12;
  if (displayHours == 0) displayHours = 12;
  
  char timeBuffer[10];
  sprintf(timeBuffer, "%02d:%02d:%02d", displayHours, timeClient.getMinutes(), timeClient.getSeconds());
  display.print(timeBuffer);

  display.setFont();
  display.setCursor(85, 20);
  display.print(rawHours >= 12 ? "PM" : "AM");
  display.drawFastHLine(0, 40, 128, SSD1306_WHITE);

  display.setFont(&FreeMono9pt7b);
  display.setCursor(5, 56);
  display.print(weekDays[timeClient.getDay()]);
}

void drawPomodoro() {
  unsigned long elapsed = (millis() - timerStartTime) / 1000;
  
  if (elapsed >= timerDuration) {
    if (currentMode == FOCUS) {
      // 1. Queue the Log
      queueLog("Focus", 25);
      
      // 2. Queue DND Off
      queueDNDChange(false);

      // 3. Switch to Break
      currentMode = BREAK;
      timerDuration = 5 * 60; 
      timerStartTime = millis();
      triggerBuzzer(2);
      
    } else {
      // Break Finished
      saveSession();
      currentMode = CLOCK;
      triggerBuzzer(3);
    }
    return;
  }

  long remaining = timerDuration - elapsed;
  int mins = remaining / 60;
  int secs = remaining % 60;

  display.setFont(); 
  display.setCursor(0, 0);
  display.print(currentMode == FOCUS ? "FOCUSING" : "BREAK TIME");

  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(25, 40);
  char timerBuf[10];
  snprintf(timerBuf, sizeof(timerBuf), "%02d:%02d", mins, secs);
  display.print(timerBuf);
  
  int progressWidth = map(elapsed, 0, timerDuration, 0, 128);
  display.drawRect(0, 55, 128, 7, SSD1306_WHITE);
  display.fillRect(0, 55, progressWidth, 7, SSD1306_WHITE);
}