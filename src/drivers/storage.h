#pragma once

#include <stdint.h>
#include <WString.h>

class Storage {
public:
    void begin();

    // Light configuration
    bool loadLightConfig(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& intensityPct);
    void saveLightConfig(uint8_t r, uint8_t g, uint8_t b, uint8_t intensityPct);

    // WiFi credentials
    bool loadWifiCredentials(String& ssid, String& pass);
    void saveWifiCredentials(const String& ssid, const String& pass);
    void resetWifiCredentials();

    // WiFi AP credentials
    bool loadApCredentials(String& ssid, String& pass);
    void saveApCredentials(const String& ssid, const String& pass);

    // WiFi Mode
    uint8_t getWifiMode();
    void setWifiMode(uint8_t mode);

    // Language setting
    uint8_t loadLanguage();
    void saveLanguage(uint8_t lang);
};