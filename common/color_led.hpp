#pragma once

#include "hardware/pio.h"

inline uint32_t rgb_to_grbz(uint32_t r, uint32_t g, uint32_t b) {
    return ((r&0xFF) << 16) | ((g&0xFF) << 24) | ((b&0xFF) << 8);
}

void rgb_to_hsv(int r, int g, int b, int& h, int& s, int& v);
void hsv_to_rgb(int h, int s, int v, int& r, int& g, int& b);

inline void put_pixel(PIO pio, uint sm, int pixel_code) {
    pio_sm_put_blocking(pio, sm, pixel_code);
}

class SerialPIO {
public:
    SerialPIO(uint pin, uint baudrate = 800000);
    ~SerialPIO();

    PIO pio() const { return pio_; }
    uint sm() const { return sm_; }
    uint pin() const { return pin_; }
    uint baudrate() const { return baudrate_; }

    void set_pin(uint pin);
    void set_baudrate(uint baudrate);
    
    void activate_program();
    void deactivate_program();

    inline void put_pixel(uint32_t pixel_code) {
        hard_assert(program_activated_);
        pio_sm_put_blocking(pio_, sm_, pixel_code);
    }
    inline void put_pixel(uint32_t pixel_code, uint32_t nled) {
        hard_assert(program_activated_);
        for(unsigned i=0; i<nled; i++) {
            pio_sm_put_blocking(pio_, sm_, pixel_code);
        }
    }

private:
    SerialPIO(const SerialPIO&) = delete;
    SerialPIO& operator=(const SerialPIO&) = delete;

    bool program_activated_ = false;
    PIO pio_;
    uint sm_;
    uint offset_; 
    uint pin_;
    uint baudrate_;
};