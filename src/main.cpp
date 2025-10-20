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
volatile bool buttonPressedFlag = false;

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
void IRAM_ATTR knobCallback(long value) {
    encoderDelta = -value;
    rotaryEncoder.resetEncoderValue();
}

void IRAM_ATTR buttonCallback(unsigned long duration) {
    // We don't use the duration, but the signature must match.
    buttonPressedFlag = true;
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
    while (true) {
        long delta = 0;
        if (encoderDelta != 0) {
            delta = encoderDelta;
            encoderDelta = 0;
        }

        bool buttonPressed = false;
        if (buttonPressedFlag) {
            buttonPressed = true;
            buttonPressedFlag = false;
        }

        lcdUi.loop(delta, buttonPressed);

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
    }

    // 3. Initialize physical UI & Encoder
    lcdUi.begin();
    rotaryEncoder.begin();
    rotaryEncoder.setBoundaries(-1000000, 1000000, false);
    rotaryEncoder.onTurned(knobCallback);
    rotaryEncoder.onPressed(buttonCallback);
    Serial.println("[main] LCD UI and Encoder initialized.");

    // 4. Initialize WiFi
    wifiManager.begin();
    Serial.println("[main] WiFi Manager initialized.");

    // 5. If WiFi connected, start the main web server and API
    if (wifiManager.isConnected()) {
        Serial.println("[main] WiFi is connected. Starting Web Server and API.");
        webServer.begin();
    } else {
        Serial.println("[main] In AP mode. Main web server not started. Configure WiFi via portal.");
    }

    // 6. Create Tasks
    xTaskCreatePinnedToCore(uiTask, "uiTask", 4096, NULL, 1, &uiTaskHandle, 0);
    xTaskCreatePinnedToCore(wifiTask, "wifiTask", 2048, NULL, 0, NULL, 1);

    Serial.println("[main] Setup complete.");
}

// ===========================================================================
// Loop
// ===========================================================================
void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
