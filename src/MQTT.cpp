#include "MQTT.h"

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

static unsigned long lastReconnectAttempt = 0;

void initMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
}

static bool reconnectMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println("[MQTT] Connected");
        return true;
    } else {
        Serial.printf("[MQTT] Connection failed, rc=%d\n", mqttClient.state());
        return false;
    }
}

void handleMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        // Non-blocking reconnect attempt every 5 seconds
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnectMQTT();
        }
    } else {
        mqttClient.loop();
    }
}

bool sendChiragButtonEvent(int buttonNumber) {
    if (!mqttClient.connected()) {
        if (!reconnectMQTT()) {
            Serial.println("[MQTT] Publish failed: Not connected");
            return false;
        }
    }

    const char* payload = NULL;
    if (buttonNumber == 1) {
        payload = "chirag_button_1";
    } else if (buttonNumber == 2) {
        payload = "chirag_button_2";
    } else if (buttonNumber == 3) {
        payload = "chirag_button_3";
    } else {
        return false;
    }

    bool success = mqttClient.publish(MQTT_TOPIC, payload);
    if (success) {
        Serial.printf("[MQTT] Published: %s\n", payload);
    } else {
        Serial.printf("[MQTT] Failed to publish: %s\n", payload);
    }
    return success;
}

bool isMQTTConnected() {
    return mqttClient.connected();
}
