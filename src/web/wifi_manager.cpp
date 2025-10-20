#include "wifi_manager.h"
#include "../config.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <memory> // Para std::unique_ptr

// =================================================================
// Captive Portal HTML
// =================================================================
const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>BioLighting WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; background-color: #f0f0f0; text-align: center; padding: 50px; }
        div { background: white; border-radius: 10px; padding: 20px; display: inline-block; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
        input { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; box-sizing: border-box; }
        button { background-color: #4CAF50; color: white; padding: 14px 20px; margin: 8px 0; border: none; border-radius: 5px; cursor: pointer; width: 100%; }
        button:hover { opacity: 0.8; }
    </style>
</head>
<body>
    <div>
        <h2>Configure WiFi</h2>
        <form action="/save" method="POST">
            <input type="text" name="ssid" placeholder="SSID" required>
            <input type="password" name="pass" placeholder="Password">
            <button type="submit">Save & Connect</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

// =================================================================
// Constructor
// =================================================================
WiFiManager::WiFiManager(Storage& storage) : _storage(storage) {
    _enabled = false;
    _currentMode = WiFiMode::STA;
}

// =================================================================
// Public API
// =================================================================
void WiFiManager::begin() {
    _enabled = _storage.isWifiEnabled();
    _currentMode = (WiFiMode)_storage.getWifiMode();
    if (_enabled) {
        if (_currentMode == WiFiMode::STA) {
            startSTAMode();
        } else {
            startAPMode();
        }
    }
}

void WiFiManager::loop() {
    if (_dnsServer) {
        _dnsServer->processNextRequest();
    }
}

bool WiFiManager::isConnected() {
    return (_currentMode == WiFiMode::STA && WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::isEnabled() {
    return _enabled;
}

WiFiMode WiFiManager::getMode() {
    return _currentMode;
}

void WiFiManager::setEnabled(bool enabled) {
    if (_enabled == enabled) return;
    _enabled = enabled;
    _storage.setWifiEnabled(enabled);

    if (_enabled) {
        if (_currentMode == WiFiMode::STA) startSTAMode();
        else startAPMode();
    } else {
        stopServers();
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        Serial.println("[WiFi] Disabled.");
    }
}

void WiFiManager::setMode(WiFiMode mode) {
    if (_currentMode == mode) return;
    _currentMode = mode;
    _storage.setWifiMode((uint8_t)mode);

    if (_enabled) {
        Serial.println("[WiFi] Mode changed. Restarting WiFi...");
        stopServers();
        if (_currentMode == WiFiMode::STA) {
            startSTAMode();
        } else {
            startAPMode();
        }
    }
}

String WiFiManager::getStaIp() {
    if (isConnected()) return WiFi.localIP().toString();
    return "0.0.0.0";
}

String WiFiManager::getApSsid() {
    String ssid;
    _storage.loadApCredentials(ssid); // Calls the single-argument overload
    return ssid;
}

String WiFiManager::getApIp() {
    if (_currentMode == WiFiMode::AP) return WiFi.softAPIP().toString();
    return "0.0.0.0";
}

String WiFiManager::getApPass() {
    return _storage.getApPass();
}

void WiFiManager::setApCredentials(const String& ssid, const String& pass) {
    _storage.saveApCredentials(ssid, pass);
    if (_enabled && _currentMode == WiFiMode::AP) {
        Serial.println("[WiFi] AP credentials updated. Restarting AP...");
        stopServers();
        startAPMode();
    }
}

// =================================================================
// Private Helpers
// =================================================================
void WiFiManager::stopServers() {
    if (_portalServer) {
        _portalServer->end();
        _portalServer.reset();
    }
    if (_dnsServer) {
        _dnsServer->stop();
        _dnsServer.reset();
    }
}

void WiFiManager::startAPMode() {
    Serial.println("[WiFi] Starting AP mode...");
    String ssid, pass;
    if (!_storage.loadApCredentials(ssid, pass)) {
        // Generate default credentials based on MAC address
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char mac_suffix[5];
        sprintf(mac_suffix, "%02X%02X", mac[4], mac[5]);
        ssid = "BioLighting-AP-" + String(mac_suffix);
        pass = "BioLight-" + String(mac_suffix);
        _storage.saveApCredentials(ssid, pass);
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), pass.c_str());
    Serial.printf("[WiFi] AP SSID: %s, IP: %s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());

    // Captive portal is only for STA setup, not for general AP mode
    // If STA credentials are not set, then we launch the portal
    String sta_ssid, sta_pass;
    if (!_storage.loadWifiCredentials(sta_ssid, sta_pass)) {
        _dnsServer = std::unique_ptr<DNSServer>(new DNSServer());
        _dnsServer->start(53, "*", WiFi.softAPIP());

        _portalServer = std::unique_ptr<AsyncWebServer>(new AsyncWebServer(80));
        _portalServer->on("/", HTTP_GET, std::bind(&WiFiManager::handleRoot, this, std::placeholders::_1));
        _portalServer->on("/save", HTTP_POST, std::bind(&WiFiManager::handleSave, this, std::placeholders::_1));
        _portalServer->onNotFound([](AsyncWebServerRequest *request) {
            request->redirect("/");
        });
        _portalServer->begin();
        Serial.println("[WiFi] Captive portal for STA setup is active.");
    }
}

void WiFiManager::startSTAMode() {
    String ssid, pass;
    if (_storage.loadWifiCredentials(ssid, pass)) {
        Serial.printf("[WiFi] Connecting to STA network: %s\n", ssid.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());

        // Non-blocking connection check can be handled by the main loop or another mechanism
        // For simplicity here, we'll keep a short blocking wait, but ideally, this
        // should be a state managed in the main loop.
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
            delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println("[WiFi] Failed to connect. Check credentials or signal.");
            // No fallback to AP mode here, that should be a user-driven choice
        }
    } else {
        Serial.println("[WiFi] No STA credentials saved. Please configure via AP mode or another method.");
        // Fallback to AP mode to allow for configuration
        setMode(WiFiMode::AP);
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

        request->send(200, "text/plain", "Credentials saved. Restarting WiFi to connect...");
        delay(500);

        // Re-initialize in STA mode with new credentials
        setMode(WiFiMode::STA);
    } else {
        request->send(400, "text/plain", "Missing SSID or password.");
    }
}
