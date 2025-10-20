#include "lcd_ui.h"
#include "../config.h"
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>
#include <WiFi.h>

// LCD I2C address and dimensions
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Debounce and timing
#define BUTTON_DEBOUNCE_MS 200
#define LONG_PRESS_MS 700

// --- Menu Definitions ---
const char* const MAIN_MENU_ITEMS[] = {"SET COLOR", "APPLY PRESET", "WIFI RESET"};
const int MAIN_MENU_SIZE = sizeof(MAIN_MENU_ITEMS) / sizeof(MAIN_MENU_ITEMS[0]);

struct Preset {
    const char* name;
    const char* short_name;
    uint8_t r, g, b;
};

const Preset PRESETS[] = {
    {"Apagado", "Off", 0, 0, 0},
    {"Crecimiento VGT", "Crecimiento", 50, 0, 255},
    {"Floracion", "Floracion", 255, 0, 100},
    {"Espectro Comp", "Completo", 255, 255, 255},
    {"Transicion", "Transicion", 180, 0, 180}
};
const int PRESET_MENU_SIZE = sizeof(PRESETS) / sizeof(PRESETS[0]);

LcdUi::LcdUi(LedDriver& ledDriver, Storage& storage) :
    _ledDriver(ledDriver),
    _storage(storage),
    _lcd(nullptr),
    _currentState(STATE_HOME),
    _r(0), _g(0), _b(0), _intensity(100),
    _language(0),
    _encoderValue(0),
    _forceRedraw(true),
    _mainMenuIndex(0),
    _presetMenuIndex(0),
    _wifiResetConfirmIndex(0)
{}

LcdUi::~LcdUi() {
    delete _lcd;
}

void LcdUi::begin() {
    _lcd = new LiquidCrystal_I2C(LCD_ADDR, LCD_COLS, LCD_ROWS);
    _lcd->init();
    _lcd->backlight();
    _lcd->clear();
    if (xSemaphoreTake(sharedVariablesMutex, pdMS_TO_TICKS(1000))) {
        _ledDriver.getColor(_r, _g, _b, _intensity);
        xSemaphoreGive(sharedVariablesMutex);
    }
}

void LcdUi::loop(long encoderDelta, bool buttonPressed) {
    if (encoderDelta != 0) {
        handleEncoder(encoderDelta);
        _forceRedraw = true;
    }
    handleButton(buttonPressed);

    if (_forceRedraw) {
        updateDisplay();
        _forceRedraw = false;
    }
}

void LcdUi::handleEncoder(long delta) {
    switch (_currentState) {
        case STATE_HOME:
            _currentState = STATE_MAIN_MENU;
            _mainMenuIndex = 0;
            break;
        case STATE_MAIN_MENU:
            _mainMenuIndex = (_mainMenuIndex + delta + MAIN_MENU_SIZE) % MAIN_MENU_SIZE;
            break;
        case STATE_APPLY_PRESET:
            _presetMenuIndex = (_presetMenuIndex + delta + PRESET_MENU_SIZE) % PRESET_MENU_SIZE;
            break;
        case STATE_WIFI_RESET_CONFIRM:
            _wifiResetConfirmIndex = (_wifiResetConfirmIndex + (delta > 0 ? 1 : -1) + 2) % 2; // Cycle 0, 1
            break;
        case STATE_SET_COLOR_R:
        case STATE_SET_COLOR_G:
        case STATE_SET_COLOR_B:
        case STATE_SET_INTENSITY:
            handleColorAdjust(delta);
            break;
        default:
            break;
    }
}

void LcdUi::handleButton(bool pressed) {
    static uint32_t lastPressTime = 0;

    if (pressed && (millis() - lastPressTime > BUTTON_DEBOUNCE_MS)) {
        lastPressTime = millis();
        _forceRedraw = true;

        // --- SHORT PRESS ---
        switch (_currentState) {
            case STATE_HOME: _currentState = STATE_MAIN_MENU; break;
            case STATE_MAIN_MENU:
                if (_mainMenuIndex == 0) _currentState = STATE_SET_COLOR;
                else if (_mainMenuIndex == 1) _currentState = STATE_APPLY_PRESET;
                else if (_mainMenuIndex == 2) _currentState = STATE_WIFI_RESET;
                break;
            case STATE_SET_COLOR: _currentState = STATE_SET_COLOR_R; break;
            case STATE_SET_COLOR_R: _currentState = STATE_SET_COLOR_G; break;
            case STATE_SET_COLOR_G: _currentState = STATE_SET_COLOR_B; break;
            case STATE_SET_COLOR_B: _currentState = STATE_SET_INTENSITY; break;
            case STATE_SET_INTENSITY:
                _currentState = STATE_HOME;
                if (xSemaphoreTake(sharedVariablesMutex, pdMS_TO_TICKS(1000))) {
                    _storage.saveLightConfig(_r, _g, _b, _intensity);
                    xSemaphoreGive(sharedVariablesMutex);
                }
                break;
            case STATE_APPLY_PRESET: {
                const Preset& p = PRESETS[_presetMenuIndex];
                _r = p.r; _g = p.g; _b = p.b;
                _intensity = (strcmp(p.name, "Apagado") == 0) ? 0 : 100;
                if (xSemaphoreTake(sharedVariablesMutex, pdMS_TO_TICKS(1000))) {
                    _ledDriver.setColor(_r, _g, _b, _intensity);
                    _storage.saveLightConfig(_r, _g, _b, _intensity);
                    xSemaphoreGive(sharedVariablesMutex);
                }
                _currentState = STATE_HOME;
                break;
            }
            case STATE_WIFI_RESET: _currentState = STATE_WIFI_RESET_CONFIRM; break;
            case STATE_WIFI_RESET_CONFIRM:
                if (_wifiResetConfirmIndex == 1) { // YES
                    _storage.resetWifiCredentials();
                    ESP.restart();
                } else { // NO
                    _currentState = STATE_HOME;
                }
                break;
        }
    }
    // Long press logic can be added here if needed
}


void LcdUi::updateDisplay() {
    // _lcd->clear(); // <-- This is the main source of flicker. Remove it.
    // Instead, each draw function is responsible for clearing its own area if needed.
    // For menus, a full redraw is fine. For dynamic data, we avoid it.

    switch (_currentState) {
        case STATE_HOME: drawHomeScreen(); break;
        case STATE_MAIN_MENU:
            _lcd->clear(); // Clear only when entering a new menu
            drawMainMenu();
            break;
        case STATE_SET_COLOR:
        case STATE_SET_COLOR_R:
        case STATE_SET_COLOR_G:
        case STATE_SET_COLOR_B:
        case STATE_SET_INTENSITY:
            drawSetColorScreen();
            break;
        case STATE_APPLY_PRESET: drawApplyPresetScreen(); break;
        case STATE_WIFI_RESET: drawMainMenu(); break; // Show the menu item before confirming
        case STATE_WIFI_RESET_CONFIRM: drawWifiResetConfirmScreen(); break;
        default: drawHomeScreen(); break;
    }
}

void LcdUi::print_line(int line, const char* text) {
    char buf[LCD_COLS + 1];
    snprintf(buf, sizeof(buf), "%-*s", LCD_COLS, text);
    _lcd->setCursor(0, line);
    _lcd->print(buf);
}

void LcdUi::drawHomeScreen() {
    static char last_line1[LCD_COLS + 1] = "";
    static char last_line2[LCD_COLS + 1] = "";
    char line1[LCD_COLS + 1];
    char line2[LCD_COLS + 1];

    // Line 1: WiFi Status
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(line1, sizeof(line1), "IP: %s", WiFi.localIP().toString().c_str());
    } else {
        snprintf(line1, sizeof(line1), "AP Mode: %s", AP_SSID);
    }

    // Line 2: Color and Intensity
    snprintf(line2, sizeof(line2), "R%03d G%03d B%03d I%03d", _r, _g, _b, _intensity);

    // Only print if the content has changed
    if (strcmp(line1, last_line1) != 0) {
        print_line(0, line1);
        strcpy(last_line1, line1);
    }
    if (strcmp(line2, last_line2) != 0) {
        print_line(1, line2);
        strcpy(last_line2, line2);
    }
}

void LcdUi::drawMainMenu() {
    print_line(0, "MENU");
    char menuItem[LCD_COLS + 1];
    snprintf(menuItem, sizeof(menuItem), "> %s", MAIN_MENU_ITEMS[_mainMenuIndex]);
    print_line(1, menuItem);
}

void LcdUi::drawApplyPresetScreen() {
    print_line(0, "APPLY PRESET");
    char presetName[LCD_COLS + 1];
    snprintf(presetName, sizeof(presetName), "> %s", PRESETS[_presetMenuIndex].short_name);
    print_line(1, presetName);
}

void LcdUi::drawWifiResetConfirmScreen() {
    print_line(0, "Reset WiFi?");
    if (_wifiResetConfirmIndex == 0) {
        print_line(1, "> NO / YES");
    } else {
        print_line(1, "  NO / > YES");
    }
}

void LcdUi::handleColorAdjust(long delta) {
    uint8_t* targetValue = nullptr;
    uint8_t max_val = 255;

    switch (_currentState) {
        case STATE_SET_COLOR_R: targetValue = &_r; break;
        case STATE_SET_COLOR_G: targetValue = &_g; break;
        case STATE_SET_COLOR_B: targetValue = &_b; break;
        case STATE_SET_INTENSITY: targetValue = &_intensity; max_val = 100; break;
        default: return;
    }

    if (targetValue) {
        int newValue = *targetValue + (delta * 5);
        if (newValue < 0) newValue = 0;
        if (newValue > max_val) newValue = max_val;
        *targetValue = newValue;
    }

    if (xSemaphoreTake(sharedVariablesMutex, pdMS_TO_TICKS(50))) {
        _ledDriver.setColor(_r, _g, _b, _intensity);
        xSemaphoreGive(sharedVariablesMutex);
    }
}

void LcdUi::drawSetColorScreen() {
    static int last_value = -1;
    static UiState last_state = STATE_HOME;

    if (_currentState == STATE_SET_COLOR) {
        _lcd->clear();
        print_line(0, "SET COLOR");
        print_line(1, "> Press to start");
        last_state = STATE_SET_COLOR;
        return;
    }

    // If the state changes (e.g., from R to G), force a redraw of the whole screen
    if (_currentState != last_state) {
        _lcd->clear();
        last_value = -1; // Force value redraw
    }

    char line1[LCD_COLS + 1];
    const char* label = "";
    int value = 0;

    switch (_currentState) {
        case STATE_SET_COLOR_R: label = "Red"; value = _r; break;
        case STATE_SET_COLOR_G: label = "Green"; value = _g; break;
        case STATE_SET_COLOR_B: label = "Blue"; value = _b; break;
        case STATE_SET_INTENSITY: label = "Intensity"; value = _intensity; break;
        default: return;
    }

    // Only update the value if it has changed
    if (value != last_value || _currentState != last_state) {
        snprintf(line1, sizeof(line1), "Set %s: %d", label, value);
        print_line(0, line1);
        last_value = value;
    }

    // The second line only needs to be drawn once per state change
    if (_currentState != last_state) {
        char line2[LCD_COLS + 1] = "";
        if (_currentState == STATE_SET_COLOR_R) strcat(line2, "R< G  B  I ");
        else if (_currentState == STATE_SET_COLOR_G) strcat(line2, "R  G< B  I ");
        else if (_currentState == STATE_SET_COLOR_B) strcat(line2, "R  G  B< I ");
        else if (_currentState == STATE_SET_INTENSITY) strcat(line2, "R  G  B  I<");
        print_line(1, line2);
        last_state = _currentState;
    }
}
const char* LcdUi::getText(int textId) {
    // This function is not used in the new implementation, but is kept for compatibility
    return "";
}
