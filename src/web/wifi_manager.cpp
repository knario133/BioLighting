#include "wifi_manager.h"
#include "../config.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

// Use a unique pointer for the server to manage its lifecycle
static std::unique_ptr<AsyncWebServer> portalServer;
static std::unique_ptr<DNSServer> dnsServer;

// Simple HTML for the captive portal
const char CAPTIVE_PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>BioShacker WiFi Setup</title>
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

WiFiManager::WiFiManager(Storage& storage) : _storage(storage) {}

void WiFiManager::begin() {
    String ssid, pass;
    if (_storage.loadWifiCredentials(ssid, pass)) {
        startSTAMode(ssid, pass);
    } else {
        startAPMode();
    }
}

void WiFiManager::loop() {
    // DNS server needs to be processed in AP mode to redirect all requests
    if (dnsServer) {
        dnsServer->processNextRequest();
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::startAPMode() {
    Serial.println("Starting AP mode for WiFi configuration.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);

    dnsServer = std::unique_ptr<DNSServer>(new DNSServer());
    dnsServer->start(53, "*", WiFi.softAPIP());

    portalServer = std::unique_ptr<AsyncWebServer>(new AsyncWebServer(80));
    portalServer->on("/", HTTP_GET, std::bind(&WiFiManager::handleRoot, this, std::placeholders::_1));
    portalServer->on("/save", HTTP_POST, std::bind(&WiFiManager::handleSave, this, std::placeholders::_1));
    // Redirect all other requests to the root
    portalServer->onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    portalServer->begin();
}

void WiFiManager::startSTAMode(const String& ssid, const String& pass) {
    Serial.printf("Connecting to saved WiFi: %s\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    // Wait for connection, with a timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\nFailed to connect. Falling back to AP mode.");
        _storage.resetWifiCredentials(); // Clear bad credentials
        startAPMode();
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

        request->send(200, "text/plain", "Credentials saved. Rebooting to connect...");

        delay(1000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Missing SSID or password.");
    }
}

// Note: handleReset is not directly used by the portal but can be exposed
// on the main web server for maintenance.
void WiFiManager::handleReset(AsyncWebServerRequest *request) {
    _storage.resetWifiCredentials();
    request->send(200, "text/plain", "WiFi credentials reset. Rebooting...");
    delay(1000);
    ESP.restart();
}
