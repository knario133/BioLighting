#include "lcd_ui.h"
#include "../config.h"
#include <LiquidCrystal_I2C.h>
#include "RotaryEncoder.h"
#include <Arduino.h>
#include <WiFi.h>

// LCD I2C address and dimensions
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Debounce and timing
#define BUTTON_DEBOUNCE_MS 50
#define LONG_PRESS_MS 700


LcdUi::LcdUi(LedDriver& ledDriver, Storage& storage) :
    _ledDriver(ledDriver),
    _storage(storage),
    _encoder(nullptr),
    _lcd(nullptr),
    _currentState(STATE_HOME),
    _r(0), _g(0), _b(0), _intensity(100),
    _encoderValue(0),
    _forceRedraw(true) {}

LcdUi::~LcdUi() {
    delete _lcd;
    delete _encoder;
}

void LcdUi::begin() {
    // Initialize LCD
    _lcd = new LiquidCrystal_I2C(LCD_ADDR, LCD_COLS, LCD_ROWS);
    _lcd->init();
    _lcd->backlight();
    _lcd->clear();

    // Initialize Encoder using pins from config.h
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
    _encoder = new RotaryEncoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, RotaryEncoder::LatchMode::FOUR3);

    // Load initial values from the LED driver
    _ledDriver.getColor(_r, _g, _b, _intensity);
    _encoderValue = 0;
}

void LcdUi::loop() {
    _encoder->tick();
    handleEncoder();
    handleButton();

    if (_forceRedraw) {
        updateDisplay();
        _forceRedraw = false;
    }
}

// Helper function for cleaner code
void print_line(LiquidCrystal_I2C* lcd, int line, const char* text) {
    char buf[LCD_COLS + 1];
    snprintf(buf, sizeof(buf), "%-*s", LCD_COLS, text);
    lcd->setCursor(0, line);
    lcd->print(buf);
}


// --- Menu Navigation and State Logic ---
// --- Menu Navigation and State Logic ---
const char* const MAIN_MENU_ITEMS[] = {"SET COLOR", "APPLY PRESET", "WIFI RESET"};
const int MAIN_MENU_SIZE = sizeof(MAIN_MENU_ITEMS) / sizeof(MAIN_MENU_ITEMS[0]);
int _mainMenuIndex = 0;

// --- Preset Definitions ---
struct Preset {
    const char* name;
    uint8_t r, g, b;
};
const Preset PRESETS[] = {
    {"Crecimiento VGT", 50, 0, 255},
    {"Floracion", 255, 0, 100},
    {"Espectro Comp", 255, 255, 255},
    {"Transicion", 180, 0, 180},
    {"APAGADO", 0, 0, 0}
};
const int PRESET_MENU_SIZE = sizeof(PRESETS) / sizeof(PRESETS[0]);
int _presetMenuIndex = 0;
int _wifiResetConfirmIndex = 0; // 0=NO, 1=YES

void LcdUi::handleEncoder() {
    long newPos = _encoder->getPosition();
    if (_encoderValue == newPos) return;

    long delta = (newPos - _encoderValue);
    _encoderValue = newPos;

    switch (_currentState) {
        case STATE_HOME: // Rotation on home screen enters the main menu
            _currentState = (UiState)(1); // Enter SET_COLOR_MENU
            _mainMenuIndex = 0;
            break;
        case STATE_SET_COLOR_MENU:
        case STATE_APPLY_PRESET:
             _mainMenuIndex = (_mainMenuIndex + delta + MAIN_MENU_SIZE) % MAIN_MENU_SIZE;
             _currentState = (UiState)(_mainMenuIndex + 1); // Offset by 1 to match enum
            break;
        case STATE_WIFI_RESET:
             _mainMenuIndex = (_mainMenuIndex + delta + MAIN_MENU_SIZE) % MAIN_MENU_SIZE;
             _currentState = (UiState)(_mainMenuIndex + 1); // Offset by 1 to match enum
            break;
        case STATE_WIFI_RESET_CONFIRM:
            _wifiResetConfirmIndex = (_wifiResetConfirmIndex + delta) % 2;
            break;

        case STATE_SET_COLOR_R:
        case STATE_SET_COLOR_G:
        case STATE_SET_COLOR_B:
        case STATE_SET_INTENSITY:
            handleColorAdjust(delta);
            break;
    }
    _forceRedraw = true;
}

void LcdUi::handleButton() {
    static uint32_t lastPressTime = 0;
    static bool wasPressed = false;
    bool isPressed = (digitalRead(ENCODER_SW_PIN) == LOW);

    if (isPressed && !wasPressed && (millis() - lastPressTime > BUTTON_DEBOUNCE_MS)) {
        lastPressTime = millis(); // Start timing for long press
    }

    if (!isPressed && wasPressed) { // Button was released
        if (millis() - lastPressTime < LONG_PRESS_MS) {
            // SHORT PRESS action
            switch (_currentState) {
                case STATE_HOME: break;
                case STATE_SET_COLOR_MENU: _currentState = STATE_SET_COLOR_R; break;
                case STATE_SET_COLOR_R: _currentState = STATE_SET_COLOR_G; break;
                case STATE_SET_COLOR_G: _currentState = STATE_SET_COLOR_B; break;
                case STATE_SET_COLOR_B: _currentState = STATE_SET_INTENSITY; break;
                case STATE_SET_INTENSITY: // Loop back to the main color menu
                    _currentState = STATE_SET_COLOR_MENU;
                    _storage.saveLightConfig(_r, _g, _b, _intensity);
                    break;
                case STATE_APPLY_PRESET: {
                    const Preset& p = PRESETS[_presetMenuIndex];
                    _r = p.r; _g = p.g; _b = p.b;
                    _intensity = (strcmp(p.name, "APAGADO") == 0) ? 0 : 100;
                    _ledDriver.setColor(_r, _g, _b, _intensity);
                    _storage.saveLightConfig(_r, _g, _b, _intensity);
                    _currentState = STATE_HOME;
                    break;
                }
                case STATE_WIFI_RESET:
                    _currentState = STATE_WIFI_RESET_CONFIRM;
                    break;
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
        _forceRedraw = true;
    } else if (isPressed && (millis() - lastPressTime > LONG_PRESS_MS)) {
        // LONG PRESS action
        _currentState = STATE_HOME; // Always return home on long press
        _forceRedraw = true;
        lastPressTime = millis(); // Prevent immediate re-trigger
    }
    wasPressed = isPressed;
}

// --- Display Drawing Functions ---
void LcdUi::updateDisplay() {
    _lcd->clear();
    switch (_currentState) {
        case STATE_HOME:
            drawHomeScreen();
            break;
        case STATE_SET_COLOR_MENU:
        case STATE_APPLY_PRESET:
            drawApplyPresetScreen();
            break;
        case STATE_WIFI_RESET:
            drawMainMenu();
            break;
        case STATE_WIFI_RESET_CONFIRM:
            drawWifiResetConfirmScreen();
            break;
        case STATE_SET_COLOR_R:
        case STATE_SET_COLOR_G:
        case STATE_SET_COLOR_B:
        case STATE_SET_INTENSITY:
            drawSetColorScreen();
            break;
        default:
            drawHomeScreen(); // Default to home
            break;
    }
}

void LcdUi::drawHomeScreen() {
    char line1[LCD_COLS + 1];
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(line1, sizeof(line1), "IP: %s", WiFi.localIP().toString().c_str());
    } else {
        snprintf(line1, sizeof(line1), "AP Mode: %s", AP_SSID);
    }
    print_line(_lcd, 0, line1);

    char line2[LCD_COLS + 1];
    snprintf(line2, sizeof(line2), "R%03d G%03d B%03d I%03d", _r, _g, _b, _intensity);
    print_line(_lcd, 1, line2);
}

void LcdUi::drawMainMenu() {
    print_line(_lcd, 0, "--- MAIN MENU ---");
    char menuItem[LCD_COLS + 1];
    snprintf(menuItem, sizeof(menuItem), "> %s", MAIN_MENU_ITEMS[_mainMenuIndex]);
    print_line(_lcd, 1, menuItem);
}

void LcdUi::drawApplyPresetScreen() {
    print_line(_lcd, 0, "--- APPLY PRESET ---");
    char presetName[LCD_COLS + 1];
    snprintf(presetName, sizeof(presetName), "> %s", PRESETS[_presetMenuIndex].name);
    print_line(_lcd, 1, presetName);
}

void LcdUi::drawWifiResetConfirmScreen() {
    print_line(_lcd, 0, "WIFI RESET: CONFIRM?");
    if (_wifiResetConfirmIndex == 0) {
        print_line(_lcd, 1, "> NO / YES");
    } else {
        print_line(_lcd, 1, "  NO / > YES");
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

    int newValue = *targetValue + (delta * 5); // Adjust by 5 per tick
    if (newValue < 0) newValue = 0;
    if (newValue > max_val) newValue = max_val;
    *targetValue = newValue;

    _ledDriver.setColor(_r, _g, _b, _intensity);
}

void LcdUi::drawSetColorScreen() {
    const char* labels[] = {"R", "G", "B", "I"};
    uint8_t values[] = {_r, _g, _b, _intensity};
    int currentIndex = _currentState - STATE_SET_COLOR_R;

    char line1[LCD_COLS + 1];
    snprintf(line1, sizeof(line1), "Set %s: %d", labels[currentIndex], values[currentIndex]);
    print_line(_lcd, 0, line1);

    char line2[LCD_COLS + 1] = "";
    for (int i = 0; i < 4; i++) {
        strcat(line2, labels[i]);
        if (i == currentIndex) strcat(line2, "< ");
        else strcat(line2, "  ");
    }
    print_line(_lcd, 1, line2);
}
