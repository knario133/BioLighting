#include "wifi_manager.h"
#include "../config.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <memory>

static std::unique_ptr<AsyncWebServer> portalServer;
static std::unique_ptr<DNSServer> dnsServer;

const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><title>BioLighting WiFi Setup</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;text-align:center;padding:50px}div{background:white;border-radius:10px;padding:20px;display:inline-block;box-shadow:0 4px 8px rgba(0,0,0,.1)}input{width:100%;padding:10px;margin:10px 0;border:1px solid #ccc;border-radius:5px;box-sizing:border-box}button{background-color:#4CAF50;color:white;padding:14px 20px;margin:8px 0;border:none;border-radius:5px;cursor:pointer;width:100%}button:hover{opacity:.8}</style></head><body><div><h2>Configure WiFi</h2><form action="/save" method="POST"><input type="text" name="ssid" placeholder="SSID" required><input type="password" name="pass" placeholder="Password"><button type="submit">Save & Connect</button></form></div></body></html>
)rawliteral";

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

String WiFiManager::getApPass() {
    String ssid, pass;
    if (_storage.loadApCredentials(ssid, pass)) {
        return pass;
    }
    // Return default password if none is stored
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char mac_suffix[5];
    sprintf(mac_suffix, "%02X%02X", mac[4], mac[5]);
    return "BioLight-" + String(mac_suffix);
}

void WiFiManager::forceApMode() {
    _storage.resetWifiCredentials();
    _currentMode = WiFiMode::AP;
    _storage.setWifiMode((uint8_t)_currentMode);
    startAPMode();
}


void WiFiManager::startAPMode() {
    Serial.println("Starting AP mode.");
    String ssid, pass;
    if (!_storage.loadApCredentials(ssid, pass)) {
        // Generate and save default credentials
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char mac_suffix[5];
        sprintf(mac_suffix, "%02X%02X", mac[4], mac[5]);
        ssid = "BioLighting-AP-" + String(mac_suffix);
        pass = "Biosync000";
        _storage.saveApCredentials(ssid, pass);
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), pass.c_str());

    dnsServer = std::unique_ptr<DNSServer>(new DNSServer());
    dnsServer->start(53, "*", WiFi.softAPIP());

    portalServer = std::unique_ptr<AsyncWebServer>(new AsyncWebServer(80));
    portalServer->on("/", HTTP_GET, std::bind(&WiFiManager::handleRoot, this, std::placeholders::_1));
    portalServer->on("/save", HTTP_POST, std::bind(&WiFiManager::handleSave, this, std::placeholders::_1));
    portalServer->onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/");
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

void WiFiManager::handleRoot(AsyncWebServerRequest *request) {
    request->send(200, "text/html", CAPTIVE_PORTAL_HTML);
}

void WiFiManager::handleSave(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
        String ssid = request->getParam("ssid", true)->value();
        String pass = request->getParam("pass", true)->value();

        _storage.saveWifiCredentials(ssid, pass);
        _storage.setWifiMode((uint8_t)WiFiMode::STA);

        request->send(200, "text/plain", "Credentials saved. Rebooting to connect...");
        delay(1000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Missing SSID or password.");
    }
}
