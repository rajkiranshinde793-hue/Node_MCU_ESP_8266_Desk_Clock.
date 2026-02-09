#include "GoogleLogger.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include "config.h"
#include "credentials.h" 

// RAM Storage for the pending log
static bool _hasPending = false;
static String _logType;
static int _logDuration;

void queueLog(String type, int durationMinutes) {
    _logType = type;
    _logDuration = durationMinutes;
    _hasPending = true;
    Serial.println("[Logger] Log Queued (RAM)");
}

bool loggerHasPending() {
    return _hasPending;
}

bool performLogUpload() {
    if (!_hasPending) return true; // Nothing to do, treat as success

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Logger] Err: WiFi not ready");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Required for HTTPS
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(HTTP_TIMEOUT_MS);

    Serial.print("[Logger] Uploading to Google Sheets...");
    
    if (http.begin(client, GOOGLE_SCRIPT_URL)) {
        http.addHeader("Content-Type", "application/json");
        String payload = "{\"type\":\"" + _logType + "\", \"duration\":\"" + String(_logDuration) + "\"}";
        
        int httpCode = http.POST(payload);
        http.end();

        if (httpCode > 0) {
            Serial.printf("Done. Code: %d\n", httpCode);
            _hasPending = false; // clear queue on success
            return true;
        } else {
            Serial.printf("Failed. Error: %s\n", http.errorToString(httpCode).c_str());
            return false; // Keep pending true to retry later
        }
    }
    return false;
}