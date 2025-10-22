#pragma once

#include <memory>
#include "rest.h"

// Forward declaration
class AsyncWebServer;

class WebServer {
public:
    enum class Mode { STA, AP };
    WebServer(RestApi& restApi);
    void begin(Mode mode);

private:
    RestApi& _restApi;
    std::unique_ptr<AsyncWebServer> _server;
};