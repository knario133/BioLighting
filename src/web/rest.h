#pragma once

#include "../drivers/led_driver.h"
#include "../drivers/storage.h"

// Forward declaration
class AsyncWebServer;

class RestApi {
public:
    RestApi(LedDriver& ledDriver, Storage& storage);
    void registerHandlers(AsyncWebServer& server);

private:
    LedDriver& _ledDriver;
    Storage& _storage;

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
};