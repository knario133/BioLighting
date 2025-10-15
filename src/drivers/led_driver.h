#pragma once

#include <stdint.h>

class LedDriver {
public:
    void initLeds();
    void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t intensityPct);
    void getColor(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& intensityPct);

private:
    uint8_t current_r = 0;
    uint8_t current_g = 0;
    uint8_t current_b = 0;
    uint8_t current_intensity = 100;
};