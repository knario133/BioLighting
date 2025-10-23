#include "web_server.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

WebServer::WebServer(RestApi& restApi) :
    _restApi(restApi),
    _server(new AsyncWebServer(80)) {}

void WebServer::begin() {
    // Register all API handlers
    _restApi.registerHandlers(*_server);

    // Serve static files from the /ui_web directory
    // The path on the server will be the root, e.g., /index.html
    _server->serveStatic("/", LittleFS, "/ui_web/").setDefaultFile("index.html");

    // --- Captive Portal and Network Config Handlers ---
    // Android sends /generate_204 for captive portal detection.
    _server->on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(204);
    });
    // Windows sends /connecttest.txt
    _server->on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Microsoft Connect Test");
    });
    // Apple sends /hotspot-detect.html
     _server->on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("http://192.168.4.1/");
    });
    // WPAD (Web Proxy Auto-Discovery Protocol) requests can be ignored.
    _server->on("/wpad.dat", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(404);
    });

    // Handle all other not-found requests by redirecting to the main page.
    // This is the core of the captive portal functionality.
    _server->onNotFound([](AsyncWebServerRequest *request){
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            Serial.printf("Redirecting not found request for %s to /index.html\n", request->url().c_str());
            request->redirect("/index.html");
        }
    });

    // Start the server
    _server->begin();
    Serial.println("Web server started.");
}
