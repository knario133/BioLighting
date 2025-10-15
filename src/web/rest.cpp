#include "rest.h"
#include "../config.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

RestApi::RestApi(LedDriver& ledDriver, Storage& storage) :
    _ledDriver(ledDriver), _storage(storage) {}

void RestApi::registerHandlers(AsyncWebServer& server) {
    // Light state handlers
    server.on("/api/light", HTTP_GET, std::bind(&RestApi::handleGetLight, this, std::placeholders::_1));

    AsyncCallbackJsonWebHandler* postLightHandler = new AsyncCallbackJsonWebHandler("/api/light",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();
            // --- Validation ---
            if (!jsonObj.containsKey("r") || !jsonObj.containsKey("g") || !jsonObj.containsKey("b") || !jsonObj.containsKey("intensity")) {
                request->send(400, "application/json", "{\"error\":\"missing_field\"}");
                return;
            }
            int r = jsonObj["r"];
            int g = jsonObj["g"];
            int b = jsonObj["b"];
            int intensity = jsonObj["intensity"];

            if (r < RGB_MIN || r > RGB_MAX || g < RGB_MIN || g > RGB_MAX || b < RGB_MIN || b > RGB_MAX || intensity < INT_MIN_PCT || intensity > INT_MAX_PCT) {
                request->send(400, "application/json", "{\"error\":\"out_of_range\"}");
                return;
            }

            // --- Apply and Save ---
            _ledDriver.setColor(r, g, b, intensity);
            _storage.saveLightConfig(r, g, b, intensity);

            // --- Respond ---
            handleGetLight(request); // Respond with the new state
        }
    );
    server.addHandler(postLightHandler);

    // Preset handlers
    server.on("/api/presets", HTTP_GET, std::bind(&RestApi::handleGetPresets, this, std::placeholders::_1));
    server.on("/api/preset/{name}", HTTP_POST, std::bind(&RestApi::handlePostPreset, this, std::placeholders::_1));

    // WiFi reset handler
    server.on("/api/wifi/reset", HTTP_POST, std::bind(&RestApi::handleWifiReset, this, std::placeholders::_1));
}

void RestApi::handleGetLight(AsyncWebServerRequest *request) {
    uint8_t r, g, b, intensity;
    _ledDriver.getColor(r, g, b, intensity);

    JsonDocument doc;
    doc["r"] = r;
    doc["g"] = g;
    doc["b"] = b;
    doc["intensity"] = intensity;

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void RestApi::handleGetPresets(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray presets = doc.to<JsonArray>();
    presets.add("warm");
    presets.add("cool");
    presets.add("sunset");

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
}

void RestApi::handlePostPreset(AsyncWebServerRequest *request) {
    String name = request->pathArg("name");
    uint8_t r, g, b;

    if (name.equalsIgnoreCase("warm")) {
        r = 255; g = 180; b = 120;
    } else if (name.equalsIgnoreCase("cool")) {
        r = 150; g = 200; b = 255;
    } else if (name.equalsIgnoreCase("sunset")) {
        r = 255; g = 100; b = 40;
    } else {
        request->send(404, "application/json", "{\"error\":\"preset_not_found\"}");
        return;
    }

    // Presets always use full intensity
    uint8_t intensity = 100;
    _ledDriver.setColor(r, g, b, intensity);
    _storage.saveLightConfig(r, g, b, intensity);

    handleGetLight(request); // Respond with the new state
}

void RestApi::handleWifiReset(AsyncWebServerRequest *request) {
    _storage.resetWifiCredentials();
    request->send(200, "text/plain", "WiFi credentials reset. Please reboot the device.");
    // No automatic restart here to allow the client to receive the response.
    // The user should manually reboot.
}
