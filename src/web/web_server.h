#pragma once

#include "rest.h"

// Forward declaration
class AsyncWebServer;

class WebServer {
public:
    WebServer(RestApi& restApi);
    void begin();

private:
    RestApi& _restApi;
    std::unique_ptr<AsyncWebServer> _server;
};