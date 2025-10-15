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

    // Language setting
    uint8_t loadLanguage();
    void saveLanguage(uint8_t lang);
};