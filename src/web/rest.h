#pragma once

#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "../drivers/led_driver.h"
#include "../drivers/storage.h"

// Forward declaration
class AsyncWebServer;

class RestApi {
public:
    RestApi(Storage& storage);
    void registerHandlers(AsyncWebServer& server);

private:
    Storage& _storage;
    String _scan_cache;
    unsigned long _last_scan_ms = 0;

    // Handlers for /api/light
    void handleGetLight(class AsyncWebServerRequest *request);
    void handlePostLight(class AsyncWebServerRequest *request, struct ArBodyHandler* handler);

    // Handlers for /api/presets
    void handleGetPresets(class AsyncWebServerRequest *request);
    void handlePostPreset(class AsyncWebServerRequest *request);

    // Handler for /api/wifi/reset
    void handleWifiReset(class AsyncWebServerRequest *request);

    // Handler for /api/wifi/status
    void handleGetWifiStatus(class AsyncWebServerRequest *request);

    // Handler for /api/wifi/scan
    void handleGetWifiScan(class AsyncWebServerRequest *request);

    // Handler for /api/wifi/connect
    void handlePostWifiConnect(class AsyncWebServerRequest *request, const JsonVariant &json);

    // Handlers for /api/lang
    void handleGetLang(class AsyncWebServerRequest *request);
    void handlePostLang(class AsyncWebServerRequest *request, const JsonVariant &json);
};
