#include <algorithm>
#include <cmath>

#include <hardware/pio.h>
#include <hardware/clocks.h>

#include "color_led.hpp"
#include "ws2812.pio.h"

void rgb_to_hsv(int ir, int ig, int ib, int& ih, int& is, int& iv)
{
    float r = ir / 255.0f;
    float g = ig / 255.0f;
    float b = ib / 255.0f;

    float max_val = std::max({r, g, b});
    float min_val = std::min({r, g, b});
    float delta = max_val - min_val;

    if (delta == 0) {
        ih = 0;
        is = 0;
        iv = static_cast<int>(max_val * 255);
        return;
    }

    if (max_val == r) {
        ih = static_cast<int>(60 * fmod((g - b) / delta, 6));
    } else if (max_val == g) {
        ih = static_cast<int>(60 * ((b - r) / delta + 2));
    } else {
        ih = static_cast<int>(60 * ((r - g) / delta + 4));
    }

    if (ih < 0) ih += 360;

    is = static_cast<int>((delta / max_val) * 255);
    iv = static_cast<int>(max_val * 255);   
}

void hsv_to_rgb(int ih, int is, int iv, int& ir, int& ig, int& ib)
{
    float r, g, b;
    float h = ih / 360.0f;
    float s = is / 255.0f;
    float v = iv / 255.0f;

    int i = static_cast<int>(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch(i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }

    ir = static_cast<int>(r * 255);
    ig = static_cast<int>(g * 255);
    ib = static_cast<int>(b * 255);
}

SerialPIO::SerialPIO(uint pin, uint baudrate): pin_(pin), baudrate_(baudrate)
{
    // nothing to see here
}

SerialPIO::~SerialPIO()
{
    if(program_activated_) {
        deactivate_program();
    }   
}

void SerialPIO::set_pin(uint pin, uint baudrate)
{
    hard_assert(!program_activated_);
    pin_ = pin;
    baudrate_ = baudrate;
}   


void SerialPIO::activate_program()
{
    hard_assert(!program_activated_);
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
        &ws2812_program, &pio_, &sm_, &offset_, pin_, 1, true);
    hard_assert(success);
    ws2812_program_init(pio_, sm_, offset_, pin_, baudrate_, false);
    program_activated_ = true;
}

void SerialPIO::deactivate_program()
{
    hard_assert(program_activated_);
    pio_remove_program_and_unclaim_sm(
        &ws2812_program, pio_, sm_, offset_);
    program_activated_ = false;
}
