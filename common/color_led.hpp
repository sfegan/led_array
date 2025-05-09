#pragma once

#include "hardware/pio.h"

inline uint32_t rgb_to_grbz(uint32_t r, uint32_t g, uint32_t b) {
    return ((r&0xFF) << 8) | ((g&0xFF) << 16) | ((b&0xFF) << 8);
}

void rgb_to_hsv(int r, int g, int b, int& h, int& s, int& v);
void hsv_to_rgb(int h, int s, int v, int& r, int& g, int& b);

inline void put_pixel(PIO pio, uint sm, int pixel_code) {
    pio_sm_put_blocking(pio, sm, pixel_code);
}
