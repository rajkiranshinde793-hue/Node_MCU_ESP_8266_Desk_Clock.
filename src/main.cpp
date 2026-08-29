#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "config.h"
#include "credentials.h"
#include "PomodoroEngine.h"
#include "ButtonManager.h"
#include "pot.h"
#include "DisplayUI.h"
#include "MQTT.h"
#include "buzzer.h"

// Wi-Fi & NTP Objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", NTP_OFFSET);

// Application State Variables
UIState currentUIState = UI_NORMAL;
NetworkState netState = NET_IDLE;

unsigned long lastSyncTime = 0;
unsigned long wifiConnectStartTime = 0;

void wakeWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP); // Disable Wi-Fi modem sleep completely
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  }
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

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("\n[SYSTEM] ESP8266 Pomodoro Desk Clock Starting...");

  // Initialize Hardware Modules
  buttonManager.init();
  potInit();
  buzzerInit();
  pomodoro.init();

  if (!displayUI.init()) {
    for (;;);
  }

  displayUI.showStartupMessage("STARTING  CHIRAG..");

  // Initial Wi-Fi & NTP Sync
  wakeWiFi();

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Connected successfully");
    displayUI.showStartupMessage("BABY JUST MOVEMENT");

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
       displayUI.showStartupMessage("NTP Failed!");
       playErrorTone();
       delay(1000);
    }
    
    initMQTT();

  } else {
    Serial.println("[WiFi] Connection Failed");
    displayUI.showStartupMessage("WiFi Failed!");
    playErrorTone();
    delay(1000);
    lastSyncTime = millis() - SYNC_INTERVAL + 10000;
  }

  currentUIState = UI_NORMAL;
}

void processEvents() {
  ButtonEvent ev = buttonManager.update();

  switch (ev) {
    case EV_BTN1_SINGLE:
      playButtonTone();
      if (currentUIState == UI_NORMAL) {
        pomodoro.startFocus();
      } else if (currentUIState == UI_TIMER_SETTINGS) {
        int highlighted = getMenuSelection(2);
        if (highlighted == 0) {
          currentUIState = UI_FOCUS_ADJUST;
        } else {
          currentUIState = UI_BREAK_ADJUST;
        }
      } else if (currentUIState == UI_FOCUS_ADJUST) {
        int mins = getFocusPresetMinutes();
        pomodoro.setFocusMinutes(mins);
        currentUIState = UI_TIMER_SETTINGS;
      } else if (currentUIState == UI_BREAK_ADJUST) {
        int mins = getBreakPresetMinutes();
        pomodoro.setBreakMinutes(mins);
        currentUIState = UI_TIMER_SETTINGS;
      }
      break;

    case EV_BTN1_DOUBLE:
      playButtonTone();
      if (currentUIState == UI_NORMAL) {
        currentUIState = UI_TIMER_SETTINGS;
      } else {
        // Exit any configuration menu back to normal screen without saving
        currentUIState = UI_NORMAL;
      }
      break;

    case EV_BTN1_LONG:
      if (currentUIState == UI_NORMAL) {
        if (pomodoro.getCurrentMode() == CLOCK) {
          pomodoro.clearSessions();
        } else {
          pomodoro.cancelTimer();
        }
      }
      break;

    case EV_BTN2_PRESS:
      Serial.println("[BUTTON] Physical Button 2");
      playButtonTone();
      sendChiragButtonEvent(1);
      break;

    case EV_BTN3_PRESS:
      Serial.println("[BUTTON] Physical Button 3");
      playButtonTone();
      sendChiragButtonEvent(2);
      break;

    case EV_BTN4_PRESS:
      Serial.println("[BUTTON] Physical Button 4");
      playButtonTone();
      sendChiragButtonEvent(3);
      break;

    case EV_NONE:
    default:
      break;
  }
}

void loop() {
  processEvents();
  pomodoro.update();
  handleMQTT();
  handleNetwork();
  handleDNDBackground();

  displayUI.update(currentUIState, timeClient);

  delay(1); // Non-blocking yield for ESP background processes
}