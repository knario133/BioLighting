#include <Arduino.h>
#include "config.h"
#include "drivers/storage.h"
#include "drivers/led_driver.h"
#include "ui_lcd/lcd_ui.h"
#include "web/wifi_manager.h"
#include "web/rest.h"
#include "web/web_server.h"
#include <ESP32RotaryEncoder.h>

// --- FreeRTOS Handles ---
SemaphoreHandle_t sharedVariablesMutex;
TaskHandle_t uiTaskHandle;

// --- Shared Global Variables ---
volatile bool uiForceRedraw = true;
volatile long encoderDelta = 0;

// --- Global Instances ---
Storage     storage;
LedDriver   ledDriver;
LcdUi       lcdUi(ledDriver, storage);
WiFiManager wifiManager(storage);
RestApi     restApi(ledDriver, storage);
WebServer   webServer(restApi);
RotaryEncoder rotaryEncoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, ENCODER_SW_PIN);

// ===========================================================================
// Interrupt Service Routines
// ===========================================================================
void IRAM_ATTR encoderCallback(long value) {
    // This is a simplistic approach. A better one might use a queue.
    // For now, we'll just update a delta.
    encoderDelta = value;
}

// ===========================================================================
// Tasks
// ===========================================================================
void wifiTask(void *parameter) {
    while (true) {
        wifiManager.loop();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void uiTask(void *parameter) {
    long lastEncoderValue = 0;

    while (true) {
        // In the original design, lcdUi.loop() handled everything.
        // We now replicate that logic here, but driven by FreeRTOS constructs.

        // 1. Handle encoder changes from the interrupt
        if (encoderDelta != lastEncoderValue) {
            long delta = encoderDelta - lastEncoderValue;
            lastEncoderValue = encoderDelta;

            // This replaces the polling part of the original handleEncoder()
            lcdUi.handleEncoderInput(delta);
            uiForceRedraw = true;
        }

        // 2. Handle button presses (still polled, but within a task)
        lcdUi.handleButton();

        // 3. Update the display only when needed
        if (uiForceRedraw) {
            lcdUi.updateDisplay();
            uiForceRedraw = false;
        }

        // Let other tasks run
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ===========================================================================
// Setup
// ===========================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[main] BioLighting Firmware Starting...");

    // 0. Create Mutex for shared variables
    sharedVariablesMutex = xSemaphoreCreateMutex();

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

    // 3. Initialize physical UI (LCD is already handled in LcdUi)
    lcdUi.begin(); // This will be simplified to just init the LCD

    // Setup Rotary Encoder with Interrupts
    rotaryEncoder.onTurned(encoderCallback);
    rotaryEncoder.begin();

    Serial.println("[main] LCD UI and Encoder initialized.");

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

    // 6. Create UI Task
    xTaskCreatePinnedToCore(
        uiTask,         // Task function
        "uiTask",       // Task name
        4096,           // Stack size
        NULL,           // Parameter
        1,              // Priority
        &uiTaskHandle,  // Task handle
        0);             // Core

    // 7. Create WiFi Task
    xTaskCreatePinnedToCore(
        wifiTask,       // Task function
        "wifiTask",     // Task name
        2048,           // Stack size
        NULL,           // Parameter
        0,              // Priority
        NULL,           // Task handle
        1);             // Core
}

// ===========================================================================
// Loop
// ===========================================================================
void loop() {
    // The main loop is now minimal. All logic is in FreeRTOS tasks.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
