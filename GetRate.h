#ifndef GET_RATE_H
#define GET_RATE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFiClientSecure.h>
#include <CertManager.h>

class GetRate {
private:
    CertManager &cert;
    WiFiClientSecure ssl_client;
    WebSocketsClient ws_client;
    String exchange_rate = "";
    String prev_tx_id = "";
    String account = "rXUMMaPpZqPutoRszR29jtC8amWq3APkx";

    void onMessageCallback(String message) {
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, message);
        if (error) {
            Serial.println("[XRPL] JSON parse error: " + String(error.c_str()));
            return;
        }

        if (doc.containsKey("result")) {
            if (doc["result"].containsKey("account_data")) {
                prev_tx_id = doc["result"]["account_data"]["PreviousTxnID"].as<String>();
                Serial.println("[XRPL] PreviousTxnID: " + prev_tx_id);
                StaticJsonDocument<256> tx_doc;
                tx_doc["id"] = 2;
                tx_doc["command"] = "tx";
                tx_doc["transaction"] = prev_tx_id;
                String tx_cmd;
                serializeJson(tx_doc, tx_cmd);
                Serial.println("[WebSocket] Sending: " + tx_cmd);
                ws_client.sendTXT(tx_cmd);
            } else if (doc["result"].containsKey("LimitAmount")) {
                exchange_rate = doc["result"]["LimitAmount"]["value"].as<String>();
                String currency = doc["result"]["LimitAmount"]["currency"].as<String>();
                String issuer = doc["result"]["LimitAmount"]["issuer"].as<String>();
                Serial.println("[XRPL] XRP/RLUSD Exchange Rate: " + exchange_rate + " (Currency: " + currency + ", Issuer: " + issuer + ")");
            }
        }
    }

    void requestTxID() {
        StaticJsonDocument<256> doc;
        doc["id"] = 1;
        doc["command"] = "account_info";
        doc["account"] = account;
        String cmd;
        serializeJson(doc, cmd);
        Serial.println("[WebSocket] Sending: " + cmd);
        ws_client.sendTXT(cmd);
    }

public:
    GetRate(CertManager &certManager) : cert(certManager) {}

    bool begin() {
        Serial.println("[WebSocket] Setting up SSL client...");
        ssl_client.setCACert(cert.getCertificate());

        Serial.println("[WebSocket] Connecting to wss://xrplcluster.com...");
        ws_client.beginSslWithCA("xrplcluster.com", 443, "/", cert.getCertificate());
        ws_client.onEvent([&](WStype_t type, uint8_t * payload, size_t length) {
            if (type == WStype_TEXT) {
                onMessageCallback(String((char*)payload));
            }
        });

        unsigned long start = millis();
        while (!ws_client.isConnected() && millis() - start < 10000) {
            ws_client.loop();
            delay(100);
        }
        if (ws_client.isConnected()) {
            Serial.println("[WebSocket] Connection Opened");
            requestTxID();
            return true;
        } else {
            Serial.println("[WebSocket] Connection failed");
            return false;
        }
    }

    String getRate() {
        if (!ws_client.isConnected()) {
            Serial.println("[WebSocket] Not connected, skipping rate fetch");
            return "";
        }
        exchange_rate = "";
        requestTxID();
        unsigned long start = millis();
        while (exchange_rate == "" && millis() - start < 10000) {
            ws_client.loop();
            delay(100);
        }
        return exchange_rate;
    }
};

#endif
