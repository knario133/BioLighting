#include "web_server.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

WebServer::WebServer(RestApi& restApi) :
    _restApi(restApi),
    _server(new AsyncWebServer(80)) {}

void WebServer::begin(Mode mode) {
    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    _restApi.registerHandlers(*_server);

    if (mode == Mode::AP) {
        // Captive portal mode
        _server->serveStatic("/", LittleFS, "/ui_web/").setDefaultFile("setup.html");
        _server->onNotFound([](AsyncWebServerRequest *request) {
            request->redirect("/setup.html");
        });
        Serial.println("Web server started in AP (Captive Portal) mode.");
    } else {
        // Standard STA mode
        _server->serveStatic("/", LittleFS, "/ui_web/").setDefaultFile("index.html");
        _server->onNotFound([](AsyncWebServerRequest *request) {
            request->send(404, "text/plain", "Not found");
        });
        Serial.println("Web server started in STA mode.");
    }

    _server->begin();
}
