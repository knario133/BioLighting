#include <Arduino.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>
#include <memory>
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
enum Screen {
    SCR_HOME,
    SCR_MENU_MAIN,
    SCR_EDIT_COLOR,
    SCR_MENU_WIFI,
    SCR_EDIT_TEXT,
    SCR_INFO
};

enum MainMenuItem {
    MAIN_MENU_COLOR,
    MAIN_MENU_WIFI,
    MAIN_MENU_LANG,
    MAIN_MENU_BACK
};

enum WifiMenuItem {
    WIFI_MENU_ENABLE,
    WIFI_MENU_MODE,
    WIFI_MENU_STA_CHANGE,
    WIFI_MENU_STA_CONNECT,
    WIFI_MENU_AP_INFO,
    WIFI_MENU_BACK
};

enum EditColorSlot {
    COLOR_SLOT_R,
    COLOR_SLOT_G,
    COLOR_SLOT_B,
    COLOR_SLOT_I // For Intensity/Brightness
};

Screen          uiScreen = SCR_HOME;
MainMenuItem    mainMenuItem = MAIN_MENU_COLOR;
WifiMenuItem    wifiMenuItem = WIFI_MENU_ENABLE;
EditColorSlot   colorSlot = COLOR_SLOT_R;
bool            editMode = false; // Will be used for text input

// State for color carousel
unsigned long lastInteractionTime = 0;
const unsigned int CAROUSEL_INTERVAL_MS = 2000;

// ===========================================================================
// i18n (Internationalization)
// ===========================================================================
enum Lang { ES, EN };
Lang currentLang = ES;

const char* TR_ES[][2] = {
  {"title", "BioLighting v2"},
  {"main.menu.color", "Color"},
  {"main.menu.wifi", "WiFi"},
  {"main.menu.lang", "Idioma"},
  {"main.menu.back", "Volver"},
  {"color.r", "R"},
  {"color.g", "G"},
  {"color.b", "B"},
  {"color.brightness", "Brillo"},
  {"hint.next", "Click: siguiente"},
  {"wifi.title", "WiFi"},
  {"wifi.on", "ON"},
  {"wifi.off", "OFF"},
  {"wifi.mode", "Modo"},
  {"wifi.mode.sta", "STA"},
  {"wifi.mode.ap", "AP"},
  {"wifi.change_network", "Cambiar Red"},
  {"wifi.connect_disconnect", "Con/Desconectar"},
  {"ap.show_info", "Info AP"},
  {"msg.connecting", "Conectando..."},
  {"msg.connected", "Conectado"},
  {"msg.ap_active", "AP activo"},
  {"msg.error", "Error"},
  {"msg.restarting_wifi", "Reiniciando WiFi"},
};

const char* TR_EN[][2] = {
  {"title", "BioLighting v2"},
  {"main.menu.color", "Color"},
  {"main.menu.wifi", "WiFi"},
  {"main.menu.lang", "Language"},
  {"main.menu.back", "Back"},
  {"color.r", "R"},
  {"color.g", "G"},
  {"color.b", "B"},
  {"color.brightness", "Brightness"},
  {"hint.next", "Click: next"},
  {"wifi.title", "WiFi"},
  {"wifi.on", "ON"},
  {"wifi.off", "OFF"},
  {"wifi.mode", "Mode"},
  {"wifi.mode.sta", "STA"},
  {"wifi.mode.ap", "AP"},
  {"wifi.change_network", "Change Network"},
  {"wifi.connect_disconnect", "Connect/Disconnect"},
  {"ap.show_info", "AP Info"},
  {"msg.connecting", "Connecting..."},
  {"msg.connected", "Connected"},
  {"msg.ap_active", "AP active"},
  {"msg.error", "Error"},
  {"msg.restarting_wifi", "Restarting WiFi"},
};

String tr(const char* key) {
    if (currentLang == EN) {
        for (unsigned int i = 0; i < sizeof(TR_EN) / sizeof(TR_EN[0]); i++) {
            if (strcmp(TR_EN[i][0], key) == 0) return TR_EN[i][1];
        }
    }
    for (unsigned int i = 0; i < sizeof(TR_ES) / sizeof(TR_ES[0]); i++) {
        if (strcmp(TR_ES[i][0], key) == 0) return TR_ES[i][1];
    }
    return "";
}

const char* mainMenuItemKey(MainMenuItem item) {
    switch(item) {
        case MAIN_MENU_COLOR: return "main.menu.color";
        case MAIN_MENU_WIFI: return "main.menu.wifi";
        case MAIN_MENU_LANG: return "main.menu.lang";
        case MAIN_MENU_BACK: return "main.menu.back";
    }
    return "";
}

const char* wifiMenuItemKey(WifiMenuItem item) {
    switch(item) {
        case WIFI_MENU_ENABLE: return "wifi.title"; // Re-using "WiFi" as the item title
        case WIFI_MENU_MODE: return "wifi.mode";
        case WIFI_MENU_STA_CHANGE: return "wifi.change_network";
        case WIFI_MENU_STA_CONNECT: return "wifi.connect_disconnect";
        case WIFI_MENU_AP_INFO: return "ap.show_info";
        case WIFI_MENU_BACK: return "main.menu.back";
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
bool needsRender = true;

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

void persistColor() {
    prefs.begin("biolight", false);
    prefs.putUChar("r", r_val);
    prefs.putUChar("g", g_val);
    prefs.putUChar("b", b_val);
    prefs.putUChar("int", intensity_val);
    prefs.end();
}

void applyDeltaToColor(int dir) {
    int delta = (dir > 0) ? 1 : -1;
    switch (colorSlot) {
        case COLOR_SLOT_R: r_val = constrain(r_val + delta * 5, 0, 255); break;
        case COLOR_SLOT_G: g_val = constrain(g_val + delta * 5, 0, 255); break;
        case COLOR_SLOT_B: b_val = constrain(b_val + delta * 5, 0, 255); break;
        case COLOR_SLOT_I: intensity_val = constrain(intensity_val + delta, 0, 100); break;
    }
    ledDriver.setColor(r_val, g_val, b_val, intensity_val);
    lastInteractionTime = millis();
    needsRender = true;
}

const char* colorSlotKey(EditColorSlot slot) {
    switch (slot) {
        case COLOR_SLOT_R: return "color.r";
        case COLOR_SLOT_G: return "color.g";
        case COLOR_SLOT_B: return "color.b";
        case COLOR_SLOT_I: return "color.brightness";
    }
    return "";
}

// ===========================================================================
// Rendering Functions
// ===========================================================================
void renderEditColor() {
    lcdClearRow(0);
    lcdClearRow(1);
    String valStr;
    switch (colorSlot) {
        case COLOR_SLOT_R: valStr = String(r_val); break;
        case COLOR_SLOT_G: valStr = String(g_val); break;
        case COLOR_SLOT_B: valStr = String(b_val); break;
        case COLOR_SLOT_I: valStr = String(intensity_val) + "%"; break;
    }
    lcdPrint16(0, "> " + tr(colorSlotKey(colorSlot)) + ": " + valStr);
    lcdPrint16(1, tr("hint.next"));
}

void renderMainMenu() {
    lcdClearRow(0);
    lcdClearRow(1);

    String line1 = (mainMenuItem == MAIN_MENU_COLOR) ? "> " : "  ";
    line1 += tr(mainMenuItemKey(MAIN_MENU_COLOR));

    String line2 = (mainMenuItem == MAIN_MENU_WIFI) ? "> " : "  ";
    line2 += tr(mainMenuItemKey(MAIN_MENU_WIFI));

    // Simple 2-item pagination for main menu
    if (mainMenuItem <= MAIN_MENU_WIFI) {
        lcdPrint16(0, line1);
        lcdPrint16(1, line2);
    } else {
        String line3 = (mainMenuItem == MAIN_MENU_LANG) ? "> " : "  ";
        line3 += tr(mainMenuItemKey(MAIN_MENU_LANG));
        String line4 = (mainMenuItem == MAIN_MENU_BACK) ? "> " : "  ";
        line4 += tr(mainMenuItemKey(MAIN_MENU_BACK));
        lcdPrint16(0, line3);
        lcdPrint16(1, line4);
    }
}

void renderInfoScreen() {
    lcdClearRow(0);
    lcdClearRow(1);

    // Simple pagination using millis() to cycle through info
    int page = (millis() / 3000) % 3;

    if (page == 0) {
        lcdPrint16(0, "AP SSID:");
        lcdPrint16(1, wifiManager.getApSsid());
    } else if (page == 1) {
        lcdPrint16(0, "AP Password:");
        lcdPrint16(1, wifiManager.getApPass());
    } else {
        lcdPrint16(0, "AP IP Address:");
        lcdPrint16(1, wifiManager.getApIp());
    }
}

void renderWifiMenu() {
    lcdClearRow(0);
    lcdClearRow(1);

    // Page 1: Common settings
    if (wifiMenuItem <= WIFI_MENU_MODE) {
        String line1 = (wifiMenuItem == WIFI_MENU_ENABLE) ? "> " : "  ";
        line1 += "WiFi: ";
        line1 += wifiManager.isEnabled() ? "[o] " + tr("wifi.on") : "[ ] " + tr("wifi.off");

        String line2 = (wifiMenuItem == WIFI_MENU_MODE) ? "> " : "  ";
        line2 += "Modo: ";
        if (wifiManager.getMode() == WiFiMode::STA) {
            line2 += "[o] STA  [ ] AP";
        } else {
            line2 += "[ ] STA  [o] AP";
        }
        lcdPrint16(0, line1);
        lcdPrint16(1, line2);
    }
    // Page 2: Mode-specific options
    else {
        if (wifiManager.getMode() == WiFiMode::STA) {
            String line1 = (wifiMenuItem == WIFI_MENU_STA_CHANGE) ? "> " : "  ";
            line1 += tr(wifiMenuItemKey(WIFI_MENU_STA_CHANGE));
            String line2 = (wifiMenuItem == WIFI_MENU_STA_CONNECT) ? "> " : "  ";
            line2 += tr(wifiMenuItemKey(WIFI_MENU_STA_CONNECT));
            lcdPrint16(0, line1);
            lcdPrint16(1, line2);
        } else { // AP Mode
            String line1 = (wifiMenuItem == WIFI_MENU_AP_INFO) ? "> " : "  ";
            line1 += tr(wifiMenuItemKey(WIFI_MENU_AP_INFO));
            String line2 = (wifiMenuItem == WIFI_MENU_BACK) ? "> " : "  ";
            line2 += tr(wifiMenuItemKey(WIFI_MENU_BACK));
            lcdPrint16(0, line1);
            lcdPrint16(1, line2);
        }
    }
}

void renderHome() {
  lcdClearRow(0);
  lcdPrint16(0, tr("title"));
  lcdClearRow(1);
  lcdPrint16(1, buildCompactLine2());
}

void render() {
    // Info screen is time-based, so it always needs rendering
    if (uiScreen == SCR_INFO) needsRender = true;
    if (!needsRender) return;

    switch (uiScreen) {
        case SCR_HOME: renderHome(); break;
        case SCR_MENU_MAIN: renderMainMenu(); break;
        case SCR_EDIT_COLOR: renderEditColor(); break;
        case SCR_MENU_WIFI: renderWifiMenu(); break;
        case SCR_INFO: renderInfoScreen(); break;
    }
    needsRender = false;
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
}

// ===========================================================================
// Encoder Click Helpers
// ===========================================================================
unsigned long clickStartTime = 0;
bool longPressTriggered = false;

void handleEncoderEvents() {
    encoder.tick();

    int dir = (int)encoder.getDirection();
    if (dir != 0) {
        lastInteractionTime = millis();
        if (uiScreen == SCR_EDIT_COLOR) {
            applyDeltaToColor(dir);
        } else if (uiScreen == SCR_MENU_MAIN) {
            int next = (int)mainMenuItem + dir;
            if (next < 0) next = (int)MAIN_MENU_BACK;
            if (next > (int)MAIN_MENU_BACK) next = (int)MAIN_MENU_COLOR;
            mainMenuItem = (MainMenuItem)next;
            needsRender = true;
        } else if (uiScreen == SCR_MENU_WIFI) {
            int current = (int)wifiMenuItem;
            int next = current + dir;

            if (dir > 0) { // Moving forward
                if (current == WIFI_MENU_MODE) {
                    next = (wifiManager.getMode() == WiFiMode::STA) ? WIFI_MENU_STA_CHANGE : WIFI_MENU_AP_INFO;
                } else if (current == WIFI_MENU_STA_CONNECT || current == WIFI_MENU_AP_INFO) {
                    next = WIFI_MENU_BACK;
                } else if (current == WIFI_MENU_BACK) {
                    next = WIFI_MENU_ENABLE;
                }
            } else { // Moving backward
                if (current == WIFI_MENU_STA_CHANGE || current == WIFI_MENU_AP_INFO) {
                    next = WIFI_MENU_MODE;
                } else if (current == WIFI_MENU_ENABLE) {
                    next = WIFI_MENU_BACK;
                }
            }
            wifiMenuItem = (WifiMenuItem)constrain(next, 0, (int)WIFI_MENU_BACK);
            needsRender = true;
        }
    }

    // --- Clicks ---
    if (digitalRead(ENCODER_SW_PIN) == LOW) {
        if (clickStartTime == 0) {
            clickStartTime = millis();
            longPressTriggered = false;
        } else if (!longPressTriggered && (millis() - clickStartTime > 1000)) {
            longPressTriggered = true;
            lastInteractionTime = millis();
            // --- Long Click Handler ---
            if (uiScreen == SCR_EDIT_COLOR) {
                persistColor();
                uiScreen = SCR_MENU_MAIN;
            } else if (uiScreen == SCR_MENU_WIFI) {
                uiScreen = SCR_MENU_MAIN;
            } else if (uiScreen == SCR_INFO) {
                uiScreen = SCR_MENU_WIFI;
            } else if (uiScreen == SCR_MENU_MAIN) {
                uiScreen = SCR_HOME;
            }
            needsRender = true;
        }
    } else {
        if (clickStartTime > 0 && !longPressTriggered) {
            lastInteractionTime = millis();
            // --- Short Click Handler ---
            if (uiScreen == SCR_HOME) {
                uiScreen = SCR_MENU_MAIN;
            } else if (uiScreen == SCR_EDIT_COLOR) {
                int nextSlot = (int)colorSlot + 1;
                if (nextSlot > (int)COLOR_SLOT_I) nextSlot = (int)COLOR_SLOT_R;
                colorSlot = (EditColorSlot)nextSlot;
            } else if (uiScreen == SCR_MENU_MAIN) {
                switch (mainMenuItem) {
                    case MAIN_MENU_COLOR: uiScreen = SCR_EDIT_COLOR; break;
                    case MAIN_MENU_WIFI: uiScreen = SCR_MENU_WIFI; break;
                    case MAIN_MENU_LANG: currentLang = (currentLang == ES) ? EN : ES; break; // Simple toggle
                    case MAIN_MENU_BACK: uiScreen = SCR_HOME; break;
                }
            } else if (uiScreen == SCR_MENU_WIFI) {
                switch (wifiMenuItem) {
                    case WIFI_MENU_ENABLE:
                        wifiManager.setEnabled(!wifiManager.isEnabled());
                        break;
                    case WIFI_MENU_MODE: {
                        WiFiMode newMode = (wifiManager.getMode() == WiFiMode::STA) ? WiFiMode::AP : WiFiMode::STA;
                        wifiManager.setMode(newMode);
                        wifiMenuItem = WIFI_MENU_ENABLE; // Reset to first page
                        break;
                    }
                    case WIFI_MENU_STA_CHANGE:
                        storage.resetWifiCredentials();
                        wifiManager.setMode(WiFiMode::AP);
                        wifiMenuItem = WIFI_MENU_ENABLE; // Go back to a safe state
                        // Here you might want to show a "restarting wifi" message
                        break;
                    case WIFI_MENU_AP_INFO:
                        uiScreen = SCR_INFO;
                        break;
                    case WIFI_MENU_BACK:
                        uiScreen = SCR_MENU_MAIN;
                        break;
                    default: break;
                }
            }
            needsRender = true;
        }
        clickStartTime = 0;
    }
}

// ===========================================================================
// Loop
// ===========================================================================
void loop() {
    wifiManager.loop();
    handleEncoderEvents();

    if (uiScreen == SCR_EDIT_COLOR) {
        if (millis() - lastInteractionTime >= CAROUSEL_INTERVAL_MS) {
            int nextSlot = (int)colorSlot + 1;
            if (nextSlot > (int)COLOR_SLOT_I) nextSlot = (int)COLOR_SLOT_R;
            colorSlot = (EditColorSlot)nextSlot;
            lastInteractionTime = millis();
            needsRender = true;
        }
    }

    render();
}
