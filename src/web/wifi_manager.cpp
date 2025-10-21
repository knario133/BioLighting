#include "wifi_manager.h"
#include "../config.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <memory>

#include <LittleFS.h>

static std::unique_ptr<AsyncWebServer> portalServer;
static std::unique_ptr<DNSServer> dnsServer;

WiFiManager::WiFiManager(Storage& storage) : _storage(storage) {
    _currentMode = WiFiMode::STA;
}

void WiFiManager::begin() {
    _currentMode = (WiFiMode)_storage.getWifiMode();
    if (_currentMode == WiFiMode::STA) {
        String ssid, pass;
        if (_storage.loadWifiCredentials(ssid, pass)) {
            startSTAMode();
        } else {
            // No credentials, switch to AP mode to allow configuration
            _currentMode = WiFiMode::AP;
            _storage.setWifiMode((uint8_t)_currentMode);
            startAPMode();
        }
    } else {
        startAPMode();
    }
}

void WiFiManager::loop() {
    if (dnsServer) {
        dnsServer->processNextRequest();
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

WiFiMode WiFiManager::getMode() {
    return _currentMode;
}

String WiFiManager::getStaIp() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WiFiManager::getApSsid() {
    String ssid, pass;
    if (_storage.loadApCredentials(ssid, pass)) {
        return ssid;
    }
    // Return default SSID if none is stored
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char mac_suffix[5];
    sprintf(mac_suffix, "%02X%02X", mac[4], mac[5]);
    return "BioLighting-AP-" + String(mac_suffix);
}

void WiFiManager::forceApMode() {
    _storage.resetWifiCredentials();
    _currentMode = WiFiMode::AP;
    _storage.setWifiMode((uint8_t)_currentMode);
    startAPMode();
}


void WiFiManager::startAPMode() {
    Serial.println("Starting AP mode.");
    String ssid, pass; // pass is unused but loadApCredentials needs it
    if (!_storage.loadApCredentials(ssid, pass)) {
        // Generate and save default credentials
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char mac_suffix[5];
        sprintf(mac_suffix, "%02X%02X", mac[4], mac[5]);
        ssid = "BioLighting-AP-" + String(mac_suffix);
        // No password for open network, just save the SSID
        _storage.saveApCredentials(ssid, "");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str()); // No password argument for open network

    dnsServer = std::unique_ptr<DNSServer>(new DNSServer());
    dnsServer->start(53, "*", WiFi.softAPIP());

    portalServer = std::unique_ptr<AsyncWebServer>(new AsyncWebServer(80));

    // Serve all static files from the root of LittleFS
    portalServer->serveStatic("/", LittleFS, "/");

    // Redirect any not-found requests to the setup page, for captive portal functionality
    portalServer->onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/setup.html");
    });

    portalServer->begin();
}

void WiFiManager::startSTAMode() {
    String ssid, pass;
    _storage.loadWifiCredentials(ssid, pass);
    Serial.printf("Connecting to saved WiFi: %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nFailed to connect. Will try again on reboot.");
    }
}
