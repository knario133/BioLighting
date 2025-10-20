#pragma once

#include <stdint.h>
#include "../drivers/led_driver.h"
#include "../drivers/storage.h"

// Forward declarations
class RotaryEncoder;
class LiquidCrystal_I2C;

class LcdUi {
public:
    LcdUi(LedDriver& ledDriver, Storage& storage);
    ~LcdUi();
    void begin();
    void loop();

private:
    // Enum for UI states
    enum UiState {
        STATE_HOME,
        STATE_SET_COLOR_MENU,
        STATE_SET_COLOR_R,
        STATE_SET_COLOR_G,
        STATE_SET_COLOR_B,
        STATE_SET_INTENSITY,
        STATE_APPLY_PRESET,
        STATE_WIFI_RESET,
        STATE_WIFI_RESET_CONFIRM
    };

    // Pointers to core components
    LedDriver& _ledDriver;
    Storage& _storage;

    // Hardware instances
    RotaryEncoder* _encoder;
    LiquidCrystal_I2C* _lcd;

    // State variables
    UiState _currentState;
    uint8_t _r, _g, _b, _intensity;
    uint8_t _language; // 0: ES, 1: EN
    long _encoderValue;
    bool _forceRedraw;

    // Private methods
    void handleEncoder();
    void handleButton();
    void updateDisplay();
    void drawMenu();
    void drawLangMenu();
    void drawWifiConnectMenu();
    void drawWifiChangeMenu();
    const char* getText(int textId);
};