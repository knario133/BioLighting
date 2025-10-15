#include "storage.h"
#include <Preferences.h>
#include "../config.h"

Preferences preferences;

void Storage::begin() {
    // Initialization is handled by the first call to preferences.begin()
}

bool Storage::loadLightConfig(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& intensityPct) {
    preferences.begin(NVS_NAMESPACE, true); // Read-only
    bool success = preferences.isKey(NVS_KEY_R);
    if (success) {
        r = preferences.getUChar(NVS_KEY_R, 0);
        g = preferences.getUChar(NVS_KEY_G, 0);
        b = preferences.getUChar(NVS_KEY_B, 0);
        intensityPct = preferences.getUChar(NVS_KEY_INT, 100);
    }
    preferences.end();
    return success;
}

void Storage::saveLightConfig(uint8_t r, uint8_t g, uint8_t b, uint8_t intensityPct) {
    preferences.begin(NVS_NAMESPACE, false); // Read-write
    preferences.putUChar(NVS_KEY_R, r);
    preferences.putUChar(NVS_KEY_G, g);
    preferences.putUChar(NVS_KEY_B, b);
    preferences.putUChar(NVS_KEY_INT, intensityPct);
    preferences.end();
}

bool Storage::loadWifiCredentials(String& ssid, String& pass) {
    preferences.begin(NVS_WIFI_NAMESPACE, true); // Read-only
    bool success = preferences.isKey(NVS_KEY_WIFI_SSID);
    if (success) {
        ssid = preferences.getString(NVS_KEY_WIFI_SSID, "");
        pass = preferences.getString(NVS_KEY_WIFI_PASS, "");
    }
    preferences.end();
    return success && ssid.length() > 0;
}

void Storage::saveWifiCredentials(const String& ssid, const String& pass) {
    preferences.begin(NVS_WIFI_NAMESPACE, false); // Read-write
    preferences.putString(NVS_KEY_WIFI_SSID, ssid);
    preferences.putString(NVS_KEY_WIFI_PASS, pass);
    preferences.end();
}

void Storage::resetWifiCredentials() {
    preferences.begin(NVS_WIFI_NAMESPACE, false);
    preferences.clear();
    preferences.end();
}

uint8_t Storage::loadLanguage() {
    preferences.begin(NVS_NAMESPACE, true);
    // 0: Spanish, 1: English. Default to 0 (Spanish).
    uint8_t lang = preferences.getUChar(NVS_KEY_LANG, 0);
    preferences.end();
    return lang;
}

void Storage::saveLanguage(uint8_t lang) {
    preferences.begin(NVS_NAMESPACE, false);
    preferences.putUChar(NVS_KEY_LANG, lang);
    preferences.end();
}
