#pragma once

// Hardware Defines
#define LED_TYPE     WS2811
#define NUM_LEDS     4
#define DATA_PIN     25

// Logical Control Range
#define RGB_MIN      0
#define RGB_MAX      255
#define INT_MIN_PCT  0
#define INT_MAX_PCT  100

// NVS Keys for settings persistence
#define NVS_NAMESPACE   "lightcfg"
#define NVS_KEY_R       "r"
#define NVS_KEY_G       "g"
#define NVS_KEY_B       "b"
#define NVS_KEY_INT     "int"

// NVS Keys for WiFi credentials
#define NVS_WIFI_NAMESPACE "wificfg"
#define NVS_KEY_WIFI_SSID  "ssid"
#define NVS_KEY_WIFI_PASS  "pass"

// Access Point configuration
#define AP_SSID "BioShacker_Conf"

// NVS Key for language
#define NVS_KEY_LANG "lang"
