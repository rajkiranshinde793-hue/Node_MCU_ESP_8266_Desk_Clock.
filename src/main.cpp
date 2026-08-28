#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h> 

#include "config.h"
#include "credentials.h"  
#include "GoogleLogger.h"
#include "DNDControl.h"
#include "buzzer.h"
#include "MQTT.h"
#include "pot.h"

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

// State Variables 
Mode currentMode = CLOCK;
NetworkState netState = NET_IDLE;

unsigned long timerStartTime = 0;
unsigned long timerDuration = 0; 
int sessionCount = 0;
unsigned long lastSyncTime = 0;
unsigned long wifiConnectStartTime = 0;

// Potentiometer & Multi-Button State Variables
int rawPotValue = 0;

bool lastBtn1State = HIGH;
unsigned long btn1PressTime = 0;

bool lastBtn2State = HIGH;
unsigned long btn2PressTime = 0;

bool lastBtn3State = HIGH;
unsigned long btn3PressTime = 0;

bool lastBtn4State = HIGH;
unsigned long btn4PressTime = 0;

// --- FUNCTION PROTOTYPES ---
void setBrightness();
void killWiFi();
void wakeWiFi();
void saveSession();
void clearSessions();
void handleNetwork();
void handleInputs();
void drawClock();
void drawPomodoro();

// --- IMPLEMENTATION ---

void setBrightness() {
  uint8_t contrast = 255; // Max brightness (0xFF)
  uint8_t precharge = 0xF1;  
  Wire.beginTransmission(0x3C);
  Wire.write(0x00);     
  Wire.write(0x81);     
  Wire.write(contrast); 
  Wire.write(0xD9);     
  Wire.write(precharge);
  Wire.endTransmission();
}

void killWiFi() {
  // Wi-Fi optimization/shutdown disabled to keep MQTT active for CHIRAG
}

void wakeWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP); // Disable Wi-Fi modem sleep completely (100% active radio)
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("\n[SYSTEM] ESP8266 Desk Clock Starting...");
  
  EEPROM.begin(4); 
  sessionCount = EEPROM.read(EEPROM_ADDR);
  if (sessionCount > MAX_SESSIONS) sessionCount = 0;

  // Initialize Hardware Pins
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_PIN, INPUT_PULLUP);
  pinMode(BUTTON_4_PIN, INPUT_PULLUP);
  potInit();

  buzzerInit();

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    for(;;);
  }
  
  display.clearDisplay();
  setBrightness();
  
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(12, 30);
  display.print("STARTING CLOCK...");
  display.display();

  // --- Initial Blocking Sync ---
  wakeWiFi();
  
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected successfully");
    display.clearDisplay();
    display.setCursor(1, 28);
    display.print("BABY JUST A MOVEMENT"); 
    display.display();

    timeClient.begin();
    
    int ntpRetry = 0;
    bool syncSuccess = false;
    
    while (ntpRetry < 10 && !syncSuccess) {
      if (timeClient.forceUpdate()) {
         if (timeClient.getEpochTime() > 946684800) {
            syncSuccess = true;
         }
      }
      if (!syncSuccess) delay(500);
      ntpRetry++;
    }

    if (syncSuccess) {
       lastSyncTime = millis();
       playSyncSuccessTune();
    } else {
       lastSyncTime = millis() - SYNC_INTERVAL + 10000;
       display.clearDisplay();
       display.setCursor(10, 30);
       display.print("NTP Failed!");
       display.display();
       playErrorTone();
       delay(1000);
    }
    
    // Initialize MQTT module
    initMQTT();

  } else {
    Serial.println("[WiFi] Connection Failed");
    display.clearDisplay();
    display.setCursor(10,30);
    display.print("WiFi Failed!");
    display.display();
    playErrorTone();
    delay(1000);
    lastSyncTime = millis() - SYNC_INTERVAL + 10000;
  }

  currentMode = CLOCK;
  
  while(digitalRead(BUTTON_1_PIN) == LOW) {
    delay(10);
  }
}

void saveSession() {
  sessionCount++;
  if (sessionCount > MAX_SESSIONS) {
    sessionCount = 0;
  }
  EEPROM.write(EEPROM_ADDR, sessionCount);
  EEPROM.commit();
}

void clearSessions() {
  sessionCount = 0;
  EEPROM.write(EEPROM_ADDR, 0);
  EEPROM.commit();
  playResetTune();
}

void handleNetwork() {
  unsigned long currentMillis = millis();
  switch (netState) {
    case NET_IDLE:
      if (currentMillis - lastSyncTime >= SYNC_INTERVAL) {
        Serial.println("Background Sync Start");
        wakeWiFi(); 
        wifiConnectStartTime = currentMillis;
        netState = NET_CONNECTING;
      }
      break;
    case NET_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        netState = NET_UPDATING;
      } 
      else if (currentMillis - wifiConnectStartTime > 15000) {
        lastSyncTime = currentMillis; 
        netState = NET_IDLE;
      }
      break;
    case NET_UPDATING:
      if (timeClient.forceUpdate()) {
         Serial.println("NTP Success");
      }
      lastSyncTime = millis();
      netState = NET_IDLE;
      break;
  }
}

void handleInputs() {
  // Read Potentiometer (Smoothed 8-sample average)
  rawPotValue = readPotSmoothed();

  // Print potentiometer value on Serial monitor when rotated/changed
  static int lastPrintedPot = -1;
  if (abs(rawPotValue - lastPrintedPot) >= 10) {
    lastPrintedPot = rawPotValue;
    int potPct = readPotPercent();
    Serial.printf("[POT] Raw: %d | Percent: %d%%\n", rawPotValue, potPct);
  }

  unsigned long now = millis();

  // --- Button 1 (Main Pomodoro / Clock Control Button: Pin D5) ---
  bool curBtn1 = digitalRead(BUTTON_1_PIN);
  if (curBtn1 == LOW && lastBtn1State == HIGH) {
    btn1PressTime = now;
  }
  
  if (curBtn1 == HIGH && lastBtn1State == LOW) {
    unsigned long pressDuration = now - btn1PressTime;
    if (pressDuration >= LONG_PRESS_MS) {
      if (currentMode == CLOCK) {
        clearSessions();
      } else {
        currentMode = CLOCK;
        timerStartTime = 0;
        playResetTune();
        queueDNDChange(false);
      }
    } else if (pressDuration > 50) {
      if (currentMode == CLOCK) {
        currentMode = FOCUS;
        timerDuration = 25 * 60; 
        timerStartTime = now;
        playFocusStartTune(); 
        queueDNDChange(true);
      }
    }
  }
  lastBtn1State = curBtn1;

  // --- Physical Button 2 (Pin D3) -> CHIRAG Logical Button 1 ---
  bool curBtn2 = digitalRead(BUTTON_2_PIN);
  if (curBtn2 == LOW && lastBtn2State == HIGH) {
    btn2PressTime = now;
    Serial.println("[BUTTON] Physical Button 2");
    playButtonTone();
    sendChiragButtonEvent(1);
  }
  lastBtn2State = curBtn2;

  // --- Physical Button 3 (Pin D4) -> CHIRAG Logical Button 2 ---
  bool curBtn3 = digitalRead(BUTTON_3_PIN);
  if (curBtn3 == LOW && lastBtn3State == HIGH) {
    btn3PressTime = now;
    Serial.println("[BUTTON] Physical Button 3");
    playButtonTone();
    sendChiragButtonEvent(2);
  }
  lastBtn3State = curBtn3;

  // --- Physical Button 4 (Pin D8) -> CHIRAG Logical Button 3 ---
  bool curBtn4 = digitalRead(BUTTON_4_PIN);
  if (curBtn4 == LOW && lastBtn4State == HIGH) {
    btn4PressTime = now;
    Serial.println("[BUTTON] Physical Button 4");
    playButtonTone();
    sendChiragButtonEvent(3);
  }
  lastBtn4State = curBtn4;
}

void loop() {
  handleInputs();
  handleMQTT();
  handleNetwork();

  handleDNDBackground();

  if (currentMode == CLOCK) {
    // Non-blocking display update (every 1 second)
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 1000) {
       lastUpdate = millis();
       display.clearDisplay();
       drawClock();
       display.display();
    }
    delay(1); 
  } else {
    // Pomodoro Mode - update frequently for responsiveness
    display.clearDisplay();
    drawPomodoro();
    display.display();
    delay(10); // Small delay to prevent WDT reset
  }
}

void drawClock() {
  display.setFont();
  display.setTextSize(1);
  display.setCursor(75, 0);
  display.print("Sess: ");
  display.print(sessionCount);

  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(5, 30); 
  int rawHours = timeClient.getHours();
  int displayHours = rawHours % 12;
  if (displayHours == 0) displayHours = 12;
  
  char timeBuffer[16];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02u:%02u:%02u", (unsigned int)displayHours, (unsigned int)timeClient.getMinutes(), (unsigned int)timeClient.getSeconds());
  display.print(timeBuffer);

  display.setFont();
  display.setCursor(85, 20);
  display.print(rawHours >= 12 ? "PM" : "AM");

  display.drawFastHLine(0, 40, 128, SSD1306_WHITE);

  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti = localtime(&rawtime);
  
  display.setFont(&FreeMono9pt7b);
  display.setCursor(5, 56);
  display.print(weekDays[timeClient.getDay()]);

  display.setCursor(70, 56);
  char dateBuffer[16];
  snprintf(dateBuffer, sizeof(dateBuffer), "%02u/%02u", (unsigned int)ti->tm_mday, (unsigned int)(ti->tm_mon + 1));
  display.print(dateBuffer);
}

void drawPomodoro() {
  unsigned long elapsed = (millis() - timerStartTime) / 1000;
  if (elapsed >= timerDuration) {
    if (currentMode == FOCUS) {

      display.clearDisplay();
      display.setCursor(10, 35);
      display.println("Logging...");
      display.display();

      logToGoogle("Focus", 25);
      queueDNDChange(false);

      currentMode = BREAK;
      timerDuration = 5 * 60; 
      timerStartTime = millis();
      playBreakStartTune();
    } else {
      saveSession();
      currentMode = CLOCK;
      playSessionCompleteTune();
    }
    return;
  }

  long remaining = timerDuration - elapsed;
  int mins = remaining / 60;
  int secs = remaining % 60;

  display.setFont(); // Switch to default small font for headers
  
  // Left: Mode Title
  display.setCursor(0, 0);
  display.print(currentMode == FOCUS ? "FOCUSING" : "BREAK TIME");

  // Right: Current Time (HH:MM)
  int pHours = timeClient.getHours();
  int pDisplayHours = pHours % 12;
  if (pDisplayHours == 0) pDisplayHours = 12;
  
  char smallTimeBuf[16];
  snprintf(smallTimeBuf, sizeof(smallTimeBuf), "%u:%02u", (unsigned int)pDisplayHours, (unsigned int)timeClient.getMinutes());
  
  // Align to right edge (approx width calculation)
  // 128 - (length * 6 pixels)
  int xPos = 128 - (strlen(smallTimeBuf) * 6);
  display.setCursor(xPos, 0);
  display.print(smallTimeBuf);

  // Main Timer Font
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(25, 40);
  char timerBuf[16];
  snprintf(timerBuf, sizeof(timerBuf), "%02u:%02u", (unsigned int)(mins > 0 ? mins : 0), (unsigned int)(secs > 0 ? secs : 0));
  display.print(timerBuf);
  
  int progressWidth = map(elapsed, 0, timerDuration, 0, 128);
  display.drawRect(0, 55, 128, 7, SSD1306_WHITE);
  display.fillRect(0, 55, progressWidth, 7, SSD1306_WHITE);
}