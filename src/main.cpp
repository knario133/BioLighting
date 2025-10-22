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
enum Screen { HOME, MENU, EDIT, WIFI_TOGGLE, WIFI_CHANGE };
enum MenuItem { M_RED, M_GREEN, M_BLUE, M_INTENSITY, M_LANG, M_WIFI_TOGGLE, M_WIFI_CHANGE, M_BACK };
enum HomeCarouselSlot { HOME_SLOT_RG, HOME_SLOT_BI };

Screen uiScreen = HOME;
MenuItem currentItem = M_RED;
HomeCarouselSlot homeSlot = HOME_SLOT_RG;
int apCarouselSlot = 0; // For AP mode carousel on line 1
unsigned long lastHomeCarouselSwitch = 0;
const int CAROUSEL_INTERVAL_MS = 2000;
bool editMode = false;

// State for interactive menus
bool wifiEnabled = true;
int wifiToggleSelection = 0; // 0: ON, 1: OFF
int wifiChangeSelection = 0; // 0: STA, 1: AP

// ===========================================================================
// i18n (Internationalization)
// ===========================================================================
enum Lang { ES, EN };
Lang currentLang = ES;

const char* TR_ES[][2] = {
  {"title", "BioLighting v2"},
  {"M_RED", "Color Rojo"},
  {"M_GREEN", "Color Verde"},
  {"M_BLUE", "Color Azul"},
  {"M_INTENSITY", "Intensidad"},
  {"M_LANG", "Idioma"},
  {"M_WIFI_TOGGLE", "WiFi Con/Des"},
  {"M_WIFI_CHANGE", "Cambiar Red"},
  {"M_BACK", "Volver"},
  {"home.red", "R"},
  {"home.green", "V"},
  {"home.blue", "A"},
  {"home.light", "I"},
  {"home.no_wifi", "SIN WIFI"},
  {"home.ap_mode", "Modo AP"},
  {"home.wifi_off", "WIFI APAGADO"},
};

const char* TR_EN[][2] = {
  {"title", "BioLighting v2"},
  {"M_RED", "Red Color"},
  {"M_GREEN", "Green Color"},
  {"M_BLUE", "Blue Color"},
  {"M_INTENSITY", "Intensity"},
  {"M_LANG", "Language"},
  {"M_WIFI_TOGGLE", "WiFi On/Off"},
  {"M_WIFI_CHANGE", "Change WiFi"},
  {"M_BACK", "Back"},
  {"home.red", "R"},
  {"home.green", "G"},
  {"home.blue", "B"},
  {"home.light", "I"},
  {"home.no_wifi", "NO WIFI"},
  {"home.ap_mode", "AP Mode"},
};

String tr(const char* key) {
    if (currentLang == EN) {
        for (int i = 0; i < sizeof(TR_EN) / sizeof(TR_EN[0]); i++) {
            if (strcmp(TR_EN[i][0], key) == 0) return TR_EN[i][1];
        }
    }
    for (int i = 0; i < sizeof(TR_ES) / sizeof(TR_ES[0]); i++) {
        if (strcmp(TR_ES[i][0], key) == 0) return TR_ES[i][1];
    }
    return "";
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
RestApi     restApi(storage);
WiFiManager wifiManager(storage);
WebServer   webServer(restApi);
Preferences prefs;
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
RotaryEncoder encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, RotaryEncoder::LatchMode::FOUR3);

uint8_t r_val, g_val, b_val, intensity_val;

// ===========================================================================
// LCD Helpers
// ===========================================================================
void lcdClearRow(uint8_t row) { lcd.setCursor(0, row); lcd.print("                "); }

void lcdPrint16(uint8_t row, const String& s) {
  String t = s;
  if (t.length() > 16) t = t.substring(0, 16);
  lcd.setCursor(0, row);
  lcd.print(t);
  for (int i = t.length(); i < 16; i++) lcd.print(" ");
}

// ===========================================================================
// Business Logic
// ===========================================================================
void persistIfNeeded() {
    prefs.begin("biolight", false);
    prefs.putUChar("lang", (uint8_t)currentLang);
    prefs.putUChar("r", r_val);
    prefs.putUChar("g", g_val);
    prefs.putUChar("b", b_val);
    prefs.putUChar("int", intensity_val);
    prefs.putBool("wifi_on", wifiEnabled);
    prefs.end();
}

void applyDeltaToCurrentItem(int dir) {
    int delta = (dir > 0) ? 1 : -1;
    switch (currentItem) {
        case M_RED: r_val = constrain(r_val + delta * 5, 0, 255); break;
        case M_GREEN: g_val = constrain(g_val + delta * 5, 0, 255); break;
        case M_BLUE: b_val = constrain(b_val + delta * 5, 0, 255); break;
        case M_INTENSITY: intensity_val = constrain(intensity_val + delta, 0, 100); break;
        case M_LANG: currentLang = (currentLang == ES) ? EN : ES; break;
        default: break;
    }
    ledDriver.setColor(r_val, g_val, b_val, intensity_val);
}

// ===========================================================================
// Rendering Functions
// ===========================================================================
void renderMenuValue() {
    lcdClearRow(1);
    String valStr;
    switch (currentItem) {
        case M_RED: valStr = String(r_val); break;
        case M_GREEN: valStr = String(g_val); break;
        case M_BLUE: valStr = String(b_val); break;
        case M_INTENSITY: valStr = String(intensity_val) + "%"; break;
        case M_LANG: valStr = (currentLang == ES) ? "Espanol" : "English"; break;
        case M_WIFI_TOGGLE: valStr = wifiEnabled ? "ON" : "OFF"; break;
        case M_WIFI_CHANGE: valStr = (wifiManager.getMode() == WiFiMode::AP) ? "AP" : "STA"; break;
        default: break;
    }
    lcdPrint16(1, (editMode ? "> " : "") + valStr);
}

void renderMenu() {
  lcdClearRow(0);
  lcdPrint16(0, tr(menuItemKey(currentItem)));
  renderMenuValue();
}

void renderWifiToggle() {
    lcdPrint16(0, tr("M_WIFI_TOGGLE"));
    String s = (wifiToggleSelection == 0) ? "[o] ON  [ ] OFF" : "[ ] ON  [o] OFF";
    lcdPrint16(1, s);
}

void renderWifiChange() {
    lcdPrint16(0, tr("M_WIFI_CHANGE"));
    String s = (wifiChangeSelection == 0) ? "[o] STA [ ] AP " : "[ ] STA [o] AP ";
    lcdPrint16(1, s);
}

void renderHome(bool forceRedraw = false) {
    static String lastLine1 = "", lastLine2 = "";

    if (forceRedraw) {
        lastLine1 = "";
        lastLine2 = "";
        lcd.clear();
    }

    String line1 = "";
    if (!wifiEnabled) {
        line1 = tr("home.wifi_off");
    } else if (wifiManager.isConnected()) {
        line1 = wifiManager.getStaIp();
    } else if (wifiManager.getMode() == WiFiMode::AP) {
        switch (apCarouselSlot) {
            case 0: line1 = tr("home.ap_mode"); break;
            case 1: line1 = wifiManager.getApSsid(); break;
            case 2: line1 = "IP: 192.168.4.1"; break;
        }
    } else {
        line1 = tr("home.no_wifi");
    }

    String line2 = "";
    char buf[17];
    if (homeSlot == HOME_SLOT_RG) {
        snprintf(buf, sizeof(buf), "%s:%-3d %s:%-3d", tr("home.red").c_str(), r_val, tr("home.green").c_str(), g_val);
    } else {
        snprintf(buf, sizeof(buf), "%s:%-3d %s:%-3d", tr("home.blue").c_str(), b_val, tr("home.light").c_str(), intensity_val);
    }
    line2 = String(buf);

    if (line1 != lastLine1) { lcdPrint16(0, line1); lastLine1 = line1; }
    if (line2 != lastLine2) { lcdPrint16(1, line2); lastLine2 = line2; }
}

// ===========================================================================
// Setup
// ===========================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[main] BioLighting Firmware Starting...");
    prefs.begin("biolight", true);
    currentLang = (Lang)prefs.getUChar("lang", (uint8_t)ES);
    r_val = prefs.getUChar("r", 255);
    g_val = prefs.getUChar("g", 255);
    b_val = prefs.getUChar("b", 255);
    intensity_val = prefs.getUChar("int", 100);
    wifiEnabled = prefs.getBool("wifi_on", true);
    prefs.end();
    ledDriver.initLeds();
    ledDriver.setColor(r_val, g_val, b_val, intensity_val);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
    if (wifiEnabled) {
        wifiManager.begin();
        if (wifiManager.getMode() == WiFiMode::STA && wifiManager.isConnected()) {
            webServer.begin(WebServer::Mode::STA);
        } else if (wifiManager.getMode() == WiFiMode::AP) {
            webServer.begin(WebServer::Mode::AP);
        }
    }
    renderHome();
    Serial.println("[main] Setup complete.");
}

// ===========================================================================
// Event Handlers
// ===========================================================================
void onShortClick() {
    if (uiScreen == HOME) {
        uiScreen = MENU;
        renderMenu();
    } else if (uiScreen == MENU && !editMode) {
        if (currentItem == M_BACK) {
            uiScreen = HOME;
            renderHome(true); // Force redraw
        } else if (currentItem == M_WIFI_TOGGLE) {
            uiScreen = WIFI_TOGGLE;
            wifiToggleSelection = wifiEnabled ? 0 : 1;
            renderWifiToggle();
        } else if (currentItem == M_WIFI_CHANGE) {
            uiScreen = WIFI_CHANGE;
            wifiChangeSelection = (wifiManager.getMode() == WiFiMode::AP);
            renderWifiChange();
        } else {
            uiScreen = EDIT;
            editMode = true;
            renderMenu();
        }
    } else if (uiScreen == EDIT) {
        uiScreen = MENU;
        editMode = false;
        persistIfNeeded();
        renderMenu();
    } else if (uiScreen == WIFI_TOGGLE) {
        bool willBeEnabled = (wifiToggleSelection == 0);
        if (willBeEnabled != wifiEnabled) {
            wifiEnabled = willBeEnabled;
            persistIfNeeded();
            if (wifiEnabled) {
                lcd.clear();
                lcdPrint16(0, "Encendiendo WiFi...");
                wifiManager.begin();
                if (wifiManager.getMode() == WiFiMode::AP) {
                    webServer.begin(WebServer::Mode::AP);
                    lcdPrint16(1, "Modo AP Activado");
                } else if (wifiManager.isConnected()) {
                    webServer.begin(WebServer::Mode::STA);
                }
                delay(2000);
            } else {
                lcd.clear();
                lcdPrint16(0, "Apagando WiFi...");
                WiFi.disconnect(true);
                delay(1000);
            }
        }
        uiScreen = HOME;
        renderHome(true);
    } else if (uiScreen == WIFI_CHANGE) {
        bool switchToAp = (wifiChangeSelection == 1);
        lcd.clear();
        if (switchToAp) {
            lcdPrint16(0, "Iniciando AP...");
            lcdPrint16(1, "Borrando red...");
            wifiManager.forceApMode();
            webServer.begin(WebServer::Mode::AP);
            delay(2000);
        } else {
            lcdPrint16(0, "Conectando WiFi...");
            wifiManager.begin();
            delay(2000);
        }
        uiScreen = HOME;
        renderHome(true);
    }
}

void onLongClick() {
    if (uiScreen == HOME) {
        uiScreen = MENU;
        renderMenu();
    } else if (uiScreen == EDIT) {
        uiScreen = MENU;
        editMode = false;
        // Reload values from prefs
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
        renderHome(true); // Force redraw
    }
}

void onEncoderTurn(int dir) {
    if (uiScreen == MENU && !editMode) {
        int n = (int)currentItem + dir;
        if (n < 0) n = (int)M_BACK;
        if (n > (int)M_BACK) n = 0;
        currentItem = (MenuItem)n;
        renderMenu();
    } else if (uiScreen == EDIT) {
        applyDeltaToCurrentItem(dir);
        renderMenuValue();
    } else if (uiScreen == WIFI_TOGGLE) {
        wifiToggleSelection = 1 - wifiToggleSelection; // Flip between 0 and 1
        renderWifiToggle();
    } else if (uiScreen == WIFI_CHANGE) {
        wifiChangeSelection = 1 - wifiChangeSelection; // Flip between 0 and 1
        renderWifiChange();
    }
}

// ===========================================================================
// Main Loop
// ===========================================================================
void loop() {
    wifiManager.loop();
    encoder.tick();

    // --- Handle Encoder Turn ---
    int dir = (int)encoder.getDirection();
    if (dir != 0) {
        onEncoderTurn(dir);
    }

    // --- Handle Encoder Click (Unified) ---
    static unsigned long pressStartTime = 0;
    static bool longPressHandled = false;
    bool isPressed = (digitalRead(ENCODER_SW_PIN) == LOW);

    if (isPressed) {
        if (pressStartTime == 0) {
            pressStartTime = millis();
        }
        if (!longPressHandled && (millis() - pressStartTime > 1000)) {
            onLongClick();
            longPressHandled = true;
        }
    } else {
        if (pressStartTime > 0 && !longPressHandled) {
            onShortClick();
        }
        pressStartTime = 0;
        longPressHandled = false;
    }

    // --- Handle Home Carousel ---
    if (uiScreen == HOME && millis() - lastHomeCarouselSwitch > CAROUSEL_INTERVAL_MS) {
        lastHomeCarouselSwitch = millis();
        // AP mode carousel for line 1
        if (wifiManager.getMode() == WiFiMode::AP) {
            apCarouselSlot = (apCarouselSlot + 1) % 3;
        }
        // General carousel for line 2
        homeSlot = (homeSlot == HOME_SLOT_RG) ? HOME_SLOT_BI : HOME_SLOT_RG;
        renderHome();
    }
}
