#include <Arduino.h>
#include "config.h"
#include "drivers/storage.h"
#include "drivers/led_driver.h"
#include "ui_lcd/lcd_ui.h"
#include "web/wifi_manager.h"
#include "web/rest.h"
#include "web/web_server.h"

// --- Global Instances ---
Storage     storage;
LedDriver   ledDriver;
LcdUi       lcdUi(ledDriver, storage);
WiFiManager wifiManager(storage);
RestApi     restApi(ledDriver, storage);
WebServer   webServer(restApi);

// ===========================================================================
// Setup
// ===========================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[main] BioLighting Firmware Starting...");

    // 1. Initialize storage
    storage.begin();

    // 2. Initialize LEDs and load last state
    ledDriver.initLeds();
    uint8_t r, g, b, intensity;
    if (storage.loadLightConfig(r, g, b, intensity)) {
        Serial.println("[main] Loaded light config from NVS.");
        ledDriver.setColor(r, g, b, intensity);
    } else {
        Serial.println("[main] No light config found in NVS. Using defaults.");
        // Default color is already set in initLeds
    }

    // 3. Initialize physical UI
    lcdUi.begin();
    Serial.println("[main] LCD UI initialized.");

    // 4. Initialize WiFi (handles AP portal or STA connection)
    wifiManager.begin();
    Serial.println("[main] WiFi Manager initialized.");

    // 5. If WiFi connected, start the main web server and API
    if (wifiManager.isConnected()) {
        Serial.println("[main] WiFi is connected. Starting Web Server and API.");
        webServer.begin();
    } else {
        Serial.println("[main] In AP mode. Main web server not started. Configure WiFi via portal.");
    }

    Serial.println("[main] Setup complete.");
}

// ===========================================================================
// Loop
// ===========================================================================
void loop() {
    // The loop is non-blocking. Each component handles its own tasks.

    // Process captive portal DNS requests if in AP mode
    wifiManager.loop();

    // Process encoder and button events for the physical UI
    lcdUi.loop();

    // A small delay to yield to other tasks, especially if the loop is very fast
    delay(10);
}
