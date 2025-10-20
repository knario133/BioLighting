#include <Arduino.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include "config.h"
#include "drivers/storage.h"
#include "drivers/led_driver.h"
#include "web/wifi_manager.h"
#include "web/rest.h"
#include "web/web_server.h"

// LCD I2C address
#define LCD_ADDR 0x27

// ===========================================================================
// UI State & Menu Definitions
// ===========================================================================
enum Screen { HOME, MENU, EDIT };
enum MenuItem { M_RED, M_GREEN, M_BLUE, M_INTENSITY, M_LANG, M_WIFI_TOGGLE, M_WIFI_CHANGE, M_BACK };

Screen uiScreen = HOME;
MenuItem currentItem = M_RED;
bool editMode = false;

// ===========================================================================
// i18n (Internationalization)
// ===========================================================================
enum Lang { ES, EN };
Lang currentLang = ES;

const char* TR_ES[][2] = {
  {"title", "BioLighting v2"},
  {"M_RED", "Rojo"},
  {"M_GREEN", "Verde"},
  {"M_BLUE", "Azul"},
  {"M_INTENSITY", "Intensidad"},
  {"M_LANG", "Idioma"},
  {"M_WIFI_TOGGLE", "WiFi Con/Des"},
  {"M_WIFI_CHANGE", "Cambiar Red"},
  {"M_BACK", "Volver"},
};

const char* TR_EN[][2] = {
  {"title", "BioLighting v2"},
  {"M_RED", "Red"},
  {"M_GREEN", "Green"},
  {"M_BLUE", "Blue"},
  {"M_INTENSITY", "Intensity"},
  {"M_LANG", "Language"},
  {"M_WIFI_TOGGLE", "WiFi On/Off"},
  {"M_WIFI_CHANGE", "Change WiFi"},
  {"M_BACK", "Back"},
};

String tr(const char* key) {
    if (currentLang == EN) {
        for (int i = 0; i < sizeof(TR_EN) / sizeof(TR_EN[0]); i++) {
            if (strcmp(TR_EN[i][0], key) == 0) return TR_EN[i][1];
        }
    }
    // Default to Spanish
    for (int i = 0; i < sizeof(TR_ES) / sizeof(TR_ES[0]); i++) {
        if (strcmp(TR_ES[i][0], key) == 0) return TR_ES[i][1];
    }
    return ""; // Not found
}

const char* menuItemKey(MenuItem item) {
    switch (item) {
        case M_RED: return "M_RED";
        case M_GREEN: return "M_GREEN";
        case M_BLUE: return "M_BLUE";
        case M_INTENSITY: return "M_INTENSITY";
        case M_LANG: return "M_LANG";
        case M_WIFI_TOGGLE: return "M_WIFI_TOGGLE";
        case M_WIFI_CHANGE: return "M_WIFI_CHANGE";
        case M_BACK: return "M_BACK";
    }
    return "";
}

// ===========================================================================
// Global Instances & State
// ===========================================================================
Storage     storage;
LedDriver   ledDriver;
WiFiManager wifiManager(storage);
RestApi     restApi(storage);
WebServer   webServer(restApi);
Preferences prefs;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
RotaryEncoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, RotaryEncoder::LatchMode::FOUR3);

uint8_t r_val, g_val, b_val, intensity_val;

// ===========================================================================
// LCD Helpers
// ===========================================================================
void lcdClearRow(uint8_t row) {
  lcd.setCursor(0, row);
  lcd.print("                ");
}

void lcdPrint16(uint8_t row, const String& s) {
  String t = s;
  if (t.length() > 16) t = t.substring(0, 16);
  lcd.setCursor(0, row);
  lcd.print(t);
  // Pad with spaces
  for (int i = t.length(); i < 16; i++) {
      lcd.print(" ");
  }
}

// ===========================================================================
// Business Logic
// ===========================================================================
String buildCompactLine2() {
  char buf[17];
  snprintf(buf, sizeof(buf), "R%03dG%03dB%03dI%03d", r_val, g_val, b_val, intensity_val);
  return String(buf);
}

void persistIfNeeded() {
    prefs.begin("biolight", false);
    prefs.putUChar("lang", (uint8_t)currentLang);
    prefs.putUChar("r", r_val);
    prefs.putUChar("g", g_val);
    prefs.putUChar("b", b_val);
    prefs.putUChar("int", intensity_val);
    prefs.end();
}

void applyDeltaToCurrentItem(int dir) {
    int delta = (dir > 0) ? 1 : -1;
    switch (currentItem) {
        case M_RED:
            r_val = constrain(r_val + delta * 5, 0, 255);
            break;
        case M_GREEN:
            g_val = constrain(g_val + delta * 5, 0, 255);
            break;
        case M_BLUE:
            b_val = constrain(b_val + delta * 5, 0, 255);
            break;
        case M_INTENSITY:
            intensity_val = constrain(intensity_val + delta, 0, 100);
            break;
        case M_LANG:
            currentLang = (currentLang == ES) ? EN : ES;
            break;
        case M_WIFI_TOGGLE: // Not implemented
            break;
        case M_WIFI_CHANGE: // Not implemented
            break;
    }
    ledDriver.setColor(r_val, g_val, b_val, intensity_val);
}

// ===========================================================================
// Rendering Functions
// ===========================================================================
void renderMenuValue() {
    lcdClearRow(1);
    String valStr = "";
    switch (currentItem) {
        case M_RED: valStr = String(r_val); break;
        case M_GREEN: valStr = String(g_val); break;
        case M_BLUE: valStr = String(b_val); break;
        case M_INTENSITY: valStr = String(intensity_val) + "%"; break;
        case M_LANG: valStr = (currentLang == ES) ? "Espanol" : "English"; break;
        default: break;
    }
    lcdPrint16(1, (editMode ? "> " : "") + valStr);
}

void renderMenu() {
  lcdClearRow(0);
  lcdPrint16(0, tr(menuItemKey(currentItem)));
  renderMenuValue();
}

void renderHome() {
  lcdClearRow(0);
  lcdPrint16(0, tr("title"));
  lcdClearRow(1);
  lcdPrint16(1, buildCompactLine2());
}

// ===========================================================================
// Setup
// ===========================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[main] BioLighting Firmware Starting...");

    prefs.begin("biolight", true); // read-only
    currentLang = (Lang)prefs.getUChar("lang", (uint8_t)ES);
    r_val = prefs.getUChar("r", 255);
    g_val = prefs.getUChar("g", 255);
    b_val = prefs.getUChar("b", 255);
    intensity_val = prefs.getUChar("int", 100);
    prefs.end();

    ledDriver.initLeds();
    ledDriver.setColor(r_val, g_val, b_val, intensity_val);

    lcd.init();
    lcd.backlight();
    lcd.clear();
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

    wifiManager.begin();
    if (wifiManager.isConnected()) {
        webServer.begin();
    }

    renderHome();
    Serial.println("[main] Setup complete.");
}

// ===========================================================================
// Encoder Click Helpers
// ===========================================================================
bool encoderShortClick() {
    static unsigned long lastClickTime = 0;
    if (digitalRead(ENCODER_SW_PIN) == LOW) {
        if (millis() - lastClickTime > 250) { // Debounce
            lastClickTime = millis();
            return true;
        }
    }
    return false;
}

bool encoderLongClick() {
    static unsigned long pressStartTime = 0;
    static bool longPressFired = false;

    if (digitalRead(ENCODER_SW_PIN) == LOW) {
        if (pressStartTime == 0) {
            pressStartTime = millis();
            longPressFired = false;
        } else if (!longPressFired && (millis() - pressStartTime > 1000)) {
            longPressFired = true;
            return true;
        }
    } else {
        pressStartTime = 0;
    }
    return false;
}

// ===========================================================================
// State Handlers
// ===========================================================================
void handleEncoderTurn(int dir) {
    if (uiScreen == MENU && !editMode) {
        int n = (int)currentItem + dir;
        if (n < (int)M_RED) n = (int)M_BACK;
        if (n > (int)M_BACK) n = (int)M_RED;
        currentItem = (MenuItem)n;
        renderMenu();
    } else if (uiScreen == EDIT || (uiScreen == MENU && editMode)) {
        applyDeltaToCurrentItem(dir);
        renderMenuValue();
    }
}

void handleShortClick() {
    if (uiScreen == HOME) {
        uiScreen = MENU;
        renderMenu();
    } else if (uiScreen == MENU && !editMode) {
        if (currentItem == M_BACK) {
            uiScreen = HOME;
            renderHome();
        } else {
            uiScreen = EDIT;
            editMode = true;
            renderMenu();
        }
    } else if (uiScreen == EDIT || (uiScreen == MENU && editMode)) {
        uiScreen = MENU;
        editMode = false;
        persistIfNeeded();
        renderMenu();
    }
}

void handleLongClick() {
    if (uiScreen == EDIT || (uiScreen == MENU && editMode)) {
        uiScreen = MENU;
        editMode = false;
        // Reload values from prefs to cancel changes
        prefs.begin("biolight", true);
        r_val = prefs.getUChar("r", 255);
        g_val = prefs.getUChar("g", 255);
        b_val = prefs.getUChar("b", 255);
        intensity_val = prefs.getUChar("int", 100);
        prefs.end();
        ledDriver.setColor(r_val, g_val, b_val, intensity_val);
        renderMenu();
    } else if (uiScreen == MENU) {
        uiScreen = HOME;
        renderHome();
    }
}

// ===========================================================================
// Loop
// ===========================================================================
void loop() {
    wifiManager.loop();
    encoder.tick();

    int dir = (int)encoder.getDirection();
    if (dir != 0) {
        handleEncoderTurn(dir);
    }
    if (encoderShortClick()) {
        handleShortClick();
    }
    if (encoderLongClick()) {
        handleLongClick();
    }
}
