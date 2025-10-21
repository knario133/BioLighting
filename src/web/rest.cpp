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
    server.on("/api/wifi/scan", HTTP_GET, std::bind(&RestApi::handleGetWifiScan, this, std::placeholders::_1));

    AsyncCallbackJsonWebHandler* postConnectHandler = new AsyncCallbackJsonWebHandler("/api/wifi/connect",
        std::bind(&RestApi::handlePostWifiConnect, this, std::placeholders::_1, std::placeholders::_2));
    server.addHandler(postConnectHandler);

    server.on("/api/lang", HTTP_GET, std::bind(&RestApi::handleGetLang, this, std::placeholders::_1));
    AsyncCallbackJsonWebHandler* postLangHandler = new AsyncCallbackJsonWebHandler("/api/lang",
        std::bind(&RestApi::handlePostLang, this, std::placeholders::_1, std::placeholders::_2));
    server.addHandler(postLangHandler);
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

void RestApi::handlePostWifiConnect(AsyncWebServerRequest *request, const JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();

    if (!jsonObj["ssid"].is<String>()) {
        request->send(200, "application/json", "{\"success\":false, \"error_code\":\"INVALID_INPUT\", \"message\":\"SSID must be a string\"}");
        return;
    }
    String ssid = jsonObj["ssid"].as<String>();
    String password = jsonObj["password"].as<String>();

    if (ssid.length() < 1 || ssid.length() > 32) {
        request->send(200, "application/json", "{\"success\":false, \"error_code\":\"INVALID_INPUT\", \"message\":\"Invalid SSID length\"}");
        return;
    }

    if (password.length() > 63) {
        request->send(200, "application/json", "{\"success\":false, \"error_code\":\"INVALID_INPUT\", \"message\":\"Password too long\"}");
        return;
    }

    Serial.printf("Connecting to WiFi network: %s\n", ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
            break; // Fail fast
        }
        delay(500);
        Serial.print(".");
    }

    JsonDocument doc;
    bool connected = (WiFi.status() == WL_CONNECTED);
    doc["success"] = connected;
    doc["mode"] = "STA";

    if (connected) {
        Serial.printf("\nSuccessfully connected to %s\n", ssid.c_str());
        _storage.saveWifiCredentials(ssid, password);
        _storage.setWifiMode(1); // 1 = STA

        doc["ip"] = WiFi.localIP().toString();
        doc["error_code"] = nullptr;
        doc["message"] = "Connected";
    } else {
        Serial.println("\nFailed to connect.");
        wl_status_t status = WiFi.status();
        WiFi.disconnect(true);

        doc["ip"] = nullptr;

        if (status == WL_NO_SSID_AVAIL) {
            doc["error_code"] = "SSID_NOT_FOUND";
            doc["message"] = "SSID not found";
        } else if (status == WL_CONNECT_FAILED) {
            doc["error_code"] = "AUTH_FAILED";
            doc["message"] = "Authentication failed";
        } else if (millis() - startTime >= 15000) {
            doc["error_code"] = "TIMEOUT";
            doc["message"] = "Connection timed out";
        } else {
            doc["error_code"] = "INTERNAL_ERROR";
            doc["message"] = "An unknown error occurred";
        }
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void RestApi::handleGetLang(AsyncWebServerRequest *request) {
    uint8_t lang = _storage.loadLanguage();
    JsonDocument doc;
    doc["lang"] = (lang == 1) ? "en" : "es"; // 1 for EN, 0 for ES
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void RestApi::handlePostLang(AsyncWebServerRequest *request, const JsonVariant &json) {
    JsonObject jsonObj = json.as<JsonObject>();
    if (!jsonObj["lang"].is<String>()) {
        request->send(400, "application/json", "{\"error\":\"invalid_input\"}");
        return;
    }
    String langStr = jsonObj["lang"].as<String>();
    uint8_t lang = 0; // Default to Spanish
    if (langStr.equalsIgnoreCase("en")) {
        lang = 1;
    }
    _storage.saveLanguage(lang);

    request->send(200, "application/json", "{\"success\":true}");
}

void RestApi::handleGetWifiScan(AsyncWebServerRequest *request) {
    bool force = request->hasParam("force") && request->getParam("force")->value() == "true";
    if (!force && _last_scan_ms > 0 && millis() - _last_scan_ms < 30000) {
        request->send(200, "application/json", _scan_cache);
        return;
    }

    int n = WiFi.scanNetworks();

    JsonDocument doc;
    doc["timestamp"] = millis() / 1000;
    JsonArray networks = doc["networks"].to<JsonArray>();

    for (int i = 0; i < n; ++i) {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        network["channel"] = WiFi.channel(i);
    }

    _scan_cache = "";
    serializeJson(doc, _scan_cache);
    _last_scan_ms = millis();

    request->send(200, "application/json", _scan_cache);
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
