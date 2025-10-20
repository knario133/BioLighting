#pragma once

#include <stdint.h>
#include <WString.h>

class Storage {
public:
    void begin();

    // Light configuration
    bool loadLightConfig(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& intensityPct);
    void saveLightConfig(uint8_t r, uint8_t g, uint8_t b, uint8_t intensityPct);

    // WiFi general config
    bool isWifiEnabled();
    void setWifiEnabled(bool enabled);
    uint8_t getWifiMode(); // 0=OFF, 1=STA, 2=AP
    void setWifiMode(uint8_t mode);

    // WiFi STA credentials
    bool loadWifiCredentials(String& ssid, String& pass);
    void saveWifiCredentials(const String& ssid, const String& pass);
    void resetWifiCredentials();

    // WiFi AP credentials
    bool loadApCredentials(String& ssid);
    bool loadApCredentials(String& ssid, String& pass);
    void saveApCredentials(const String& ssid, const String& pass);
    String getApPass();

    // Language setting
    uint8_t loadLanguage();
    void saveLanguage(uint8_t lang);
};