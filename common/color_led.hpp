#pragma once

#include "pico/time.h"
#include "hardware/pio.h"

#include "menu.hpp"

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
    SerialPIO(int pin, int baudrate = 800000);
    ~SerialPIO();

    int pin() const { return pin_; }
    int baudrate() const { return baudrate_; }
    int nled() const { return nled_; }
    int non() const { return non_; }
    bool back() const { return back_; }

    void set_pin(int pin);
    void set_baudrate(int baudrate);
    void set_nled(int nled) { nled_ = nled; }
    void set_non(int non) { non_ = non; }
    void set_back(bool back) { back_ = back; }

    PIO pio() const { return pio_; }
    uint sm() const { return sm_; }
    bool program_activated() const { return program_activated_; }

    void activate_program();
    void deactivate_program();

    inline void put_pixel(uint32_t pixel_code) const {
        hard_assert(program_activated_);
        pio_sm_put_blocking(pio_, sm_, pixel_code);
    }
    inline void put_pixel(uint32_t pixel_code, uint32_t nled) const {
        hard_assert(program_activated_);
        for(unsigned i=0; i<nled; i++) {
            pio_sm_put_blocking(pio_, sm_, pixel_code);
        }
    }
    inline void end_of_string() const {
        hard_assert(program_activated_);
        for(unsigned iloop=0; iloop<1000000 and !pio_sm_is_tx_fifo_empty(pio_, sm_); ++iloop) {
            // wait for the TX FIFO to be empty
        }
            busy_wait_us(50);
    }

protected:
    SerialPIO(const SerialPIO&) = delete;
    SerialPIO& operator=(const SerialPIO&) = delete;

    int pin_ = 28;
    int baudrate_ = 800000;
    int nled_ = 0;
    int non_ = 0;
    bool back_ = false;

    bool program_activated_ = false;
    PIO pio_;
    uint sm_;
    uint offset_;  
};

class SerialPIOMenu: public SerialPIO, public SimpleItemValueMenu {
public:
    SerialPIOMenu(int pin, int baudrate = 800000);
    ~SerialPIOMenu() override;

    bool event_loop_starting(int& return_code) override;
    void event_loop_finishing(int& return_code) override;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters,
        absolute_time_t& next_timer) override;
    bool process_timer(bool controller_is_connected, int& return_code,
        absolute_time_t& next_timer) override;

private:
    enum MenuItemPositions {
        MIP_PIN,
        MIP_BAUDRATE,
        MIP_NLED,
        MIP_NON,
        MIP_BACK,
        MIP_LAMP_TEST,
        MIP_EXIT,
        MIP_NUM_ITEMS // MUST BE LAST ITEM IN LIST
    };

    void enable_lamp_test();
    void disable_lamp_test();
    void send_color_string();

    void set_pin_value(bool draw = true);
    void set_baudrate_value(bool draw = true);
    void set_nled_value(bool draw = true);
    void set_non_value(bool draw = true);
    void set_back_value(bool draw = true);
    void set_lamp_test_value(bool draw = true);
    
    std::vector<MenuItem> make_menu_items();

private:
    int lamp_test_cycle_ = -1;
    int lamp_test_count_ = 0;
    unsigned heartbeat_timer_count_ = 0;
};

#define MAX_PIXELS 32*8*8
#define WS2812_DEFAULT_BAUDRATE 800000
#define WS2812_DEFAULT_PIN 28
