#include "DNDControl.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include "config.h"
#include "credentials.h" 

static bool _dndPending = false;
static bool _targetState = false;

void queueDNDChange(bool enable) {
    _targetState = enable;
    _dndPending = true;
    Serial.printf("[DND] Queued State: %s\n", enable ? "ON" : "OFF");
}

bool dndHasPending() {
    return _dndPending;
}

bool performDndSync() {
    if (!_dndPending) return true;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[DND] Err: WiFi not ready");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    
    const char* targetURL = _targetState ? URL_DND_ON : URL_DND_OFF;
    
    Serial.print("[DND] Triggering Webhook...");
    
    http.begin(client, targetURL);
    int code = http.GET();
    http.end();
    
    if (code > 0) {
        Serial.printf("Done. Code: %d\n", code);
        _dndPending = false;
        return true;
    } else {
        Serial.printf("Failed. Error: %s\n", http.errorToString(code).c_str());
        return false;
    }
}