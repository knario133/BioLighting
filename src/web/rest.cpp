#include "rest.h"
#include "../config.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

extern LedDriver ledDriver;
extern Preferences prefs;
extern uint8_t r_val, g_val, b_val, intensity_val;

RestApi::RestApi(Storage& storage) : _storage(storage) {}

void RestApi::registerHandlers(AsyncWebServer& server) {
    // Light state handlers
    server.on("/api/light", HTTP_GET, std::bind(&RestApi::handleGetLight, this, std::placeholders::_1));

    AsyncCallbackJsonWebHandler* postLightHandler = new AsyncCallbackJsonWebHandler("/api/light",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonObject jsonObj = json.as<JsonObject>();
            if (!jsonObj["r"].is<int>() || !jsonObj["g"].is<int>() || !jsonObj["b"].is<int>() || !jsonObj["intensity"].is<int>()) {
                request->send(400, "application/json", "{\"error\":\"missing_field\"}");
                return;
            }
            r_val = jsonObj["r"];
            g_val = jsonObj["g"];
            b_val = jsonObj["b"];
            intensity_val = jsonObj["intensity"];

            if (r_val < 0 || r_val > 255 || g_val < 0 || g_val > 255 || b_val < 0 || b_val > 255 || intensity_val < 0 || intensity_val > 100) {
                request->send(400, "application/json", "{\"error\":\"out_of_range\"}");
                return;
            }

            ledDriver.setColor(r_val, g_val, b_val, intensity_val);
            prefs.begin("biolight", false);
            prefs.putUChar("r", r_val);
            prefs.putUChar("g", g_val);
            prefs.putUChar("b", b_val);
            prefs.putUChar("int", intensity_val);
            prefs.end();

            handleGetLight(request);
        }
    );
    server.addHandler(postLightHandler);

    server.on("/api/presets", HTTP_GET, std::bind(&RestApi::handleGetPresets, this, std::placeholders::_1));
    server.on("/api/preset/{name}", HTTP_POST, std::bind(&RestApi::handlePostPreset, this, std::placeholders::_1));
    server.on("/api/wifi/reset", HTTP_POST, std::bind(&RestApi::handleWifiReset, this, std::placeholders::_1));
    server.on("/api/wifi/status", HTTP_GET, std::bind(&RestApi::handleGetWifiStatus, this, std::placeholders::_1));
}

void RestApi::handleGetLight(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["r"] = r_val;
    doc["g"] = g_val;
    doc["b"] = b_val;
    doc["intensity"] = intensity_val;

    String json;
    serializeJson(doc, json);
    request->send(200, String("application/json"), json);
}

void RestApi::handleGetPresets(AsyncWebServerRequest *request) {
    JsonDocument doc;
    JsonArray presets = doc.to<JsonArray>();
    presets.add("warm");
    presets.add("cool");
    presets.add("sunset");

    String json;
    serializeJson(doc, json);
    request->send(200, String("application/json"), json);
}

void RestApi::handlePostPreset(AsyncWebServerRequest *request) {
    String name = request->pathArg(0);

    if (name.equalsIgnoreCase("warm")) {
        r_val = 255; g_val = 180; b_val = 120;
    } else if (name.equalsIgnoreCase("cool")) {
        r_val = 150; g_val = 200; b_val = 255;
    } else if (name.equalsIgnoreCase("sunset")) {
        r_val = 255; g_val = 100; b_val = 40;
    } else {
        request->send(404, "application/json", "{\"error\":\"preset_not_found\"}");
        return;
    }

    intensity_val = 100;
    ledDriver.setColor(r_val, g_val, b_val, intensity_val);
    prefs.begin("biolight", false);
    prefs.putUChar("r", r_val);
    prefs.putUChar("g", g_val);
    prefs.putUChar("b", b_val);
    prefs.putUChar("int", intensity_val);
    prefs.end();

    handleGetLight(request);
}

void RestApi::handleWifiReset(AsyncWebServerRequest *request) {
    _storage.resetWifiCredentials();
    request->send(200, "text/plain", "WiFi credentials reset. Please reboot the device.");
}

void RestApi::handleGetWifiStatus(AsyncWebServerRequest *request) {
    JsonDocument doc;
    bool isConnected = (WiFi.status() == WL_CONNECTED);

    doc["wifi"] = isConnected;
    doc["mode"] = isConnected ? "STA" : (WiFi.getMode() == WIFI_AP ? "AP" : "OFF");

    if (isConnected) {
        doc["ip"] = WiFi.localIP().toString();
        doc["ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();
    } else {
        doc["ip"] = WiFi.softAPIP().toString();
        doc["ssid"] = "";
        doc["rssi"] = 0;
    }

    String json;
    serializeJson(doc, json);
    request->send(200, String("application/json"), json);
}
