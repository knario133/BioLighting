#include "led_driver.h"
#include <FastLED.h>
#include "../config.h"

// Define the array of leds
CRGB leds[NUM_LEDS];

void LedDriver::initLeds() {
    FastLED.addLeds<LED_TYPE, DATA_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(255); // Start at max brightness, intensity will scale it
    setColor(0, 0, 0, 100);   // Default to off but full intensity
}

void LedDriver::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t intensityPct) {
    // Store current state
    current_r = r;
    current_g = g;
    current_b = b;
    current_intensity = intensityPct;

    // Map intensity (0-100) to brightness (0-255)
    uint8_t brightness = map(intensityPct, INT_MIN_PCT, INT_MAX_PCT, 0, 255);
    FastLED.setBrightness(brightness);

    // Set color for all LEDs
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(r, g, b);
    }

    // Apply changes
    FastLED.show();
}

void LedDriver::getColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& intensityPct) {
    r = current_r;
    g = current_g;
    b = current_b;
    intensityPct = current_intensity;
}
