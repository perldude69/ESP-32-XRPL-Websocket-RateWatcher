#ifndef CERT_MANAGER_H
#define CERT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPIFFS.h>

class CertManager {
private:
    const char* cert_file = "/xrplcluster.pem";
    const char* cert_update_url = "https://192.168.0.105/xrplcluster.pem";
    char cert_buffer[2048];

    // Initialize SPIFFS with retries
    bool initSPIFFS() {
        Serial.println("[CertManager] Initializing SPIFFS...");
        for (int i = 0; i < 3; i++) {
            if (SPIFFS.begin(true)) {
                Serial.println("[CertManager] SPIFFS initialized");
                return true;
            }
            Serial.println("[CertManager] SPIFFS attempt " + String(i + 1) + " failed, retrying...");
            delay(500); // Yield to WDT
        }
        Serial.println("[CertManager] Failed to mount SPIFFS after retries");
        return false;
    }

    // Update certificate from URL
    bool updateCertificates(const char* url, const char* filePath, const char* rootCA = NULL) {
        Serial.println("[CertManager] Checking WiFi connection...");
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[CertManager] WiFi not connected");
            return false;
        }

        if (!initSPIFFS()) {
            return false;
        }

        Serial.println("[CertManager] Starting HTTP request...");
        HTTPClient http;
        WiFiClientSecure client;
        if (rootCA) {
            client.setCACert(rootCA);
        } else {
            client.setInsecure();
            Serial.println("[CertManager] Warning: Skipping server certificate verification");
        }

        http.begin(client, url);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            Serial.println("[CertManager] HTTP request succeeded, writing file...");
            File file = SPIFFS.open(filePath, "w");
            if (!file) {
                Serial.println("[CertManager] Failed to open file for writing");
                http.end();
                return false;
            }

            WiFiClient* stream = http.getStreamPtr();
            uint8_t buff[128];
            size_t total = 0;
            while (http.connected() && (total < http.getSize() || http.getSize() == -1)) {
                size_t len = stream->readBytes(buff, sizeof(buff));
                if (len == 0) break;
                if (file.write(buff, len) != len) {
                    Serial.println("[CertManager] Failed to write to file");
                    file.close();
                    http.end();
                    return false;
                }
                total += len;
            }
            Serial.println("[CertManager] File written successfully");
            file.close();
        } else {
            Serial.printf("[CertManager] HTTP GET failed, error code: %d\n", httpCode);
            http.end();
            return false;
        }

        http.end();
        return true;
    }

public:
    CertManager() {
        cert_buffer[0] = '\0';
    }

    // Update and load certificate
    bool setCertificates() {
        Serial.println("[CertManager] Setting certificates...");
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[CertManager] WiFi not connected, skipping certificate setup");
            return false;
        }
        if (!initSPIFFS()) {
            return false;
        }

        Serial.println("[CertManager] Updating certificate...");
        if (updateCertificates(cert_update_url, cert_file)) {
            Serial.println("[CertManager] Certificate updated");
        } else {
            Serial.println("[CertManager] Certificate update failed");
        }

        Serial.println("[CertManager] Loading certificate...");
        File file = SPIFFS.open(cert_file, "r");
        if (!file) {
            Serial.println("[CertManager] Failed to open certificate file");
            return false;
        }
        size_t len = file.readBytes(cert_buffer, sizeof(cert_buffer) - 1);
        cert_buffer[len] = '\0';
        file.close();
        Serial.println("[CertManager] Certificate loaded, length: " + String(len));
        return len > 0;
    }

    // Get the loaded certificate
    const char* getCertificate() {
        return cert_buffer;
    }
};

#endif
