#pragma once

#include <memory>
#include "../drivers/storage.h"

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

enum class WiFiMode { OFF, STA, AP };

class WiFiManager {
public:
    WiFiManager(Storage& storage);
    void begin();
    void loop();

    // --- Public Control API ---
    bool isConnected();
    void setEnabled(bool enabled);
    void setMode(WiFiMode mode);
    bool isEnabled();
    WiFiMode getMode();
    String getStaIp();
    String getApSsid();
    String getApIp();
    String getApPass();
    void setApCredentials(const String& ssid, const String& pass);
    void connect();
    void disconnect();

private:
    Storage& _storage;
    WiFiMode _currentMode;
    bool _enabled;

    std::unique_ptr<DNSServer> _dnsServer;
    std::unique_ptr<AsyncWebServer> _portalServer;

    void stopServers();
    void startAPMode();
    void startSTAMode();
    void handleRoot(AsyncWebServerRequest *request);
    void handleSave(AsyncWebServerRequest *request);
};