#ifndef SMART_WIFI_H
#define SMART_WIFI_H

#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsync_WiFiManager.h>
#include <ESPAsyncDNSServer.h>

class SmartWiFi {
private:
    const char* wifi_config_file = "/wifi.dat";
    AsyncWebServer server;
    AsyncDNSServer dns;
    ESPAsync_WiFiManager wifiManager;

    // Save WiFi credentials to SPIFFS
    bool saveCredentials(String ssid, String password) {
        Serial.println("[SmartWiFi] Saving credentials...");
        File file = SPIFFS.open(wifi_config_file, "w");
        if (!file) {
            Serial.println("[SmartWiFi] Failed to open wifi.dat for writing");
            return false;
        }
        file.println(ssid);
        file.println(password);
        file.close();
        Serial.println("[SmartWiFi] WiFi credentials saved");
        return true;
    }

    // Load WiFi credentials from SPIFFS
    bool loadCredentials(String &ssid, String &password) {
        Serial.println("[SmartWiFi] Loading credentials...");
        File file = SPIFFS.open(wifi_config_file, "r");
        if (!file) {
            Serial.println("[SmartWiFi] No wifi.dat found, starting config portal");
            return false;
        }
        ssid = file.readStringUntil('\n');
        ssid.trim();
        password = file.readStringUntil('\n');
        password.trim();
        file.close();
        Serial.println("[SmartWiFi] WiFi credentials loaded");
        return true;
    }

    // Initialize SPIFFS with retries
    bool initSPIFFS() {
        Serial.println("[SmartWiFi] Initializing SPIFFS...");
        for (int i = 0; i < 3; i++) {
            if (SPIFFS.begin(true)) {
                Serial.println("[SmartWiFi] SPIFFS initialized");
                return true;
            }
            Serial.println("[SmartWiFi] SPIFFS attempt " + String(i + 1) + " failed, retrying...");
            delay(500); // Yield to WDT
        }
        Serial.println("[SmartWiFi] Failed to mount SPIFFS after retries");
        return false;
    }

public:
    SmartWiFi() : server(80), wifiManager(&server, &dns, "ESP32_Config") {
        Serial.println("[SmartWiFi] Constructor called");
    }

    // Initialize WiFi and SPIFFS
    bool begin() {
        Serial.println("[SmartWiFi] Begin started...");
        if (!initSPIFFS()) {
            Serial.println("[SmartWiFi] SPIFFS initialization failed");
            return false;
        }

        String ssid, password;
        if (loadCredentials(ssid, password)) {
            Serial.println("[SmartWiFi] Connecting to WiFi with saved credentials...");
            WiFi.begin(ssid.c_str(), password.c_str());
            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
                Serial.print(".");
                delay(500); // Yield to WDT
            }
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[SmartWiFi] Starting config portal...");
            wifiManager.setConfigPortalTimeout(180); // 3 minutes timeout
            if (!wifiManager.startConfigPortal("ESP32_Config")) {
                Serial.println("[SmartWiFi] Config portal failed");
                return false;
            }
            saveCredentials(WiFi.SSID(), WiFi.psk());
        }

        Serial.println("[SmartWiFi] Connected, IP: " + WiFi.localIP().toString());
        Serial.println("[SmartWiFi] RSSI: " + String(WiFi.RSSI()));
        Serial.println("[SmartWiFi] Station ID (MAC): " + WiFi.macAddress());
        return true;
    }

    // Check and reconnect WiFi
    bool check() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[SmartWiFi] Disconnected, attempting to reconnect...");
            WiFi.reconnect();
            unsigned long start = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
                Serial.print(".");
                delay(500); // Yield to WDT
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("[SmartWiFi] Reconnected, IP: " + WiFi.localIP().toString());
                Serial.println("[SmartWiFi] RSSI: " + String(WiFi.RSSI()));
                Serial.println("[SmartWiFi] Station ID (MAC): " + WiFi.macAddress());
                return true;
            }
            Serial.println("[SmartWiFi] Reconnection failed");
            return false;
        }
        return true;
    }
};

#endif
