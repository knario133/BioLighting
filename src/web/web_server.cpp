#include "web_server.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

WebServer::WebServer(RestApi& restApi) :
    _restApi(restApi),
    _server(new AsyncWebServer(80)) {}

void WebServer::begin() {
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    // Register all API handlers
    _restApi.registerHandlers(*_server);

    // Serve static files from the /ui_web directory
    // The path on the server will be the root, e.g., /index.html
    _server->serveStatic("/", LittleFS, "/ui_web/").setDefaultFile("index.html");

    // Handle 404s
    _server->onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });

    // Start the server
    _server->begin();
    Serial.println("Web server started.");
}
