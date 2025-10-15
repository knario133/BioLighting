#pragma once

#include "../drivers/storage.h"

class WiFiManager {
public:
    WiFiManager(Storage& storage);
    void begin();
    void loop();
    bool isConnected();

private:
    Storage& _storage;
    void startAPMode();
    void startSTAMode(const String& ssid, const String& pass);
    void handleRoot(class AsyncWebServerRequest *request);
    void handleSave(class AsyncWebServerRequest *request);
    void handleReset(class AsyncWebServerRequest *request);
};