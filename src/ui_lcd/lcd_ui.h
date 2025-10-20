#pragma once

#include <stdint.h>
#include "../drivers/led_driver.h"
#include "../drivers/storage.h"
#include <Arduino.h>

// Forward declarations
class RotaryEncoder;
class LiquidCrystal_I2C;

// --- Extern the mutex handle defined in main.cpp ---
extern SemaphoreHandle_t sharedVariablesMutex;

class LcdUi {
public:
    LcdUi(LedDriver& ledDriver, Storage& storage);
    ~LcdUi();
    void begin();

    // --- Public methods for task-based operation ---
    void handleEncoderInput(long delta);
    void handleButton();
    void updateDisplay();

private:
    // Enum for UI states
    enum UiState {
        STATE_HOME,
    STATE_MAIN_MENU,
    STATE_SET_COLOR,
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
    LiquidCrystal_I2C* _lcd;

    // State variables
    UiState _currentState;
    uint8_t _r, _g, _b, _intensity;
    uint8_t _language; // 0: ES, 1: EN
    long _encoderValue;
    bool _forceRedraw;
    int _mainMenuIndex;
    int _presetMenuIndex;
    int _wifiResetConfirmIndex;

    // Private methods
void drawHomeScreen();
void drawMainMenu();
void drawSetColorScreen();
void drawApplyPresetScreen();
void drawWifiResetConfirmScreen();
void handleColorAdjust(long value);
void print_line(int line, const char* text);
    const char* getText(int textId);
};