#include "lcd_ui.h"
#include "../config.h"
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include <Arduino.h>

// Pin definitions from the reference project
#define I2C_SDA 21
#define I2C_SCL 22
#define ENC_DT 18  // Corresponds to PIN_IN1 for RotaryEncoder
#define ENC_CLK 5 // Corresponds to PIN_IN2 for RotaryEncoder
#define ENC_SW 19   // Button pin

// Debounce delay for the button
#define BUTTON_DEBOUNCE_MS 200

LcdUi::LcdUi(LedDriver& ledDriver, Storage& storage) :
    _ledDriver(ledDriver),
    _storage(storage),
    _encoder(nullptr),
    _lcd(nullptr),
    _currentState(MENU_R),
    _r(0), _g(0), _b(0), _intensity(100),
    _language(0),
    _encoderValue(0),
    _forceRedraw(true) {}

void LcdUi::begin() {
    // Load language first
    _language = _storage.loadLanguage();

    // Initialize LCD
    _lcd = new LiquidCrystal_I2C(0x27, 16, 2);
    _lcd->init();
    _lcd->backlight();
    _lcd->clear();

    // Initialize Encoder
    // NOTE: The RotaryEncoder library doesn't handle the button itself.
    // We will read it manually with INPUT_PULLUP.
    pinMode(ENC_SW, INPUT_PULLUP);
    _encoder = new RotaryEncoder(ENC_DT, ENC_CLK, RotaryEncoder::LatchMode::FOUR3);

    // Load initial values from the LED driver (which should have loaded from NVS)
    _ledDriver.getColor(_r, _g, _b, _intensity);
    _encoderValue = 0; // Reset encoder position
}

void LcdUi::loop() {
    handleEncoder();
    handleButton();
    if (_forceRedraw) {
        updateDisplay();
    }
}

// --- Text Definitions for i18n ---
enum TextId {
    TXT_RED,
    TXT_GREEN,
    TXT_BLUE,
    TXT_INTENSITY,
    TXT_LANGUAGE,
    TXT_VALUE,
    TXT_LANG_ES,
    TXT_LANG_EN
};

const char* const TEXTS_ES[] = {
    "Rojo", "Verde", "Azul", "Intensidad", "Idioma", "Valor", "Espanol", "Ingles"
};
const char* const TEXTS_EN[] = {
    "Red", "Green", "Blue", "Intensity", "Language", "Value", "Spanish", "English"
};

const char* LcdUi::getText(int textId) {
    if (_language == 1) { // English
        return TEXTS_EN[textId];
    }
    return TEXTS_ES[textId]; // Spanish
}


void LcdUi::handleEncoder() {
    _encoder->tick();
    long newPos = _encoder->getPosition();

    if (_encoderValue != newPos) {
        long delta = newPos - _encoderValue;
        _encoderValue = newPos;

        if (_currentState == MENU_LANG) {
            _language = (_language + delta) % 2;
            if (_language < 0) _language = 1;
             _storage.saveLanguage(_language);
        } else {
            int* targetValue = nullptr;
            int max_val = 255;

            switch (_currentState) {
                case MENU_R: targetValue = (int*)&_r; break;
                case MENU_G: targetValue = (int*)&_g; break;
                case MENU_B: targetValue = (int*)&_b; break;
                case MENU_INTENSITY: targetValue = (int*)&_intensity; max_val = 100; break;
                default: break;
            }

            if (targetValue) {
                int newValue = *targetValue + delta;
                if (newValue < 0) newValue = 0;
                if (newValue > max_val) newValue = max_val;
                *targetValue = newValue;

                _ledDriver.setColor(_r, _g, _b, _intensity);
            }
        }
        _forceRedraw = true;
    }
}

void LcdUi::handleButton() {
    static uint32_t lastButtonPress = 0;
    if (digitalRead(ENC_SW) == LOW && (millis() - lastButtonPress > BUTTON_DEBOUNCE_MS)) {
        lastButtonPress = millis();

        if (_currentState == MENU_LANG) {
            _storage.saveLanguage(_language);
        } else {
            _storage.saveLightConfig(_r, _g, _b, _intensity);
        }

        // Cycle to the next state (now 5 states)
        _currentState = (UiState)((_currentState + 1) % 5);
        _forceRedraw = true;
    }
}

void LcdUi::updateDisplay() {
    _lcd->clear();
    if (_currentState == MENU_LANG) {
        drawLangMenu();
    } else {
        drawMenu();
    }
    _forceRedraw = false;
}

void LcdUi::drawMenu() {
    char line[17];
    const char* label = "";
    int value = 0;

    switch (_currentState) {
        case MENU_R: label = getText(TXT_RED); value = _r; break;
        case MENU_G: label = getText(TXT_GREEN); value = _g; break;
        case MENU_B: label = getText(TXT_BLUE); value = _b; break;
        case MENU_INTENSITY: label = getText(TXT_INTENSITY); value = _intensity; break;
        default: break;
    }

    snprintf(line, sizeof(line), "> %s", label);
    _lcd->setCursor(0, 0);
    _lcd->print(line);

    if (_currentState == MENU_INTENSITY) {
        snprintf(line, sizeof(line), "  %s: %3d %%", getText(TXT_VALUE), value);
    } else {
        snprintf(line, sizeof(line), "  %s: %3d", getText(TXT_VALUE), value);
    }
    _lcd->setCursor(0, 1);
    _lcd->print(line);
}

void LcdUi::drawLangMenu() {
    char line[17];
    _lcd->setCursor(0, 0);
    snprintf(line, sizeof(line), "> %s", getText(TXT_LANGUAGE));
    _lcd->print(line);

    _lcd->setCursor(0, 1);
    if (_language == 0) { // Spanish
        snprintf(line, sizeof(line), "> %s", getText(TXT_LANG_ES));
    } else { // English
        snprintf(line, sizeof(line), "> %s", getText(TXT_LANG_EN));
    }
    _lcd->print(line);
}
