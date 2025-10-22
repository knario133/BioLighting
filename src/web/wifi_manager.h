#pragma once

#include "../drivers/storage.h"

enum class WiFiMode { OFF, STA, AP };

class WiFiManager {
public:
    WiFiManager(Storage& storage);
    void begin();
    void loop();
    bool isConnected();
    WiFiMode getMode();
    String getStaIp();
    String getApSsid();
    void forceApMode();

private:
    Storage& _storage;
    WiFiMode _currentMode;
    void startAPMode();
    void startSTAMode();
};