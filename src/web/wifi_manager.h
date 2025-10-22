#pragma once

#include "../drivers/storage.h"
#include "rest.h"

enum class WiFiMode { OFF, STA, AP };

class RestApi;

class WiFiManager {
public:
    WiFiManager(Storage& storage, RestApi& restApi);
    void begin();
    void loop();
    bool isConnected();
    WiFiMode getMode();
    String getStaIp();
    String getApSsid();
    void forceApMode();

private:
    Storage& _storage;
    RestApi& _restApi;
    WiFiMode _currentMode;
    void startAPMode();
    void startSTAMode();
    void handleRoot(class AsyncWebServerRequest *request);
    void handleSave(class AsyncWebServerRequest *request);
};