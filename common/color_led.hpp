#pragma once

#include "pico/time.h"
#include "hardware/pio.h"

#include "menu.hpp"

inline uint32_t rgb_to_grbz(uint32_t r, uint32_t g, uint32_t b) {
    return ((r&0xFF) << 16) | ((g&0xFF) << 24) | ((b&0xFF) << 8);
}

void rgb_to_hsv(int r, int g, int b, int& h, int& s, int& v);
void hsv_to_rgb(int h, int s, int v, int& r, int& g, int& b);

class RGBHSVMenuItems {
public:
    RGBHSVMenuItems(SimpleItemValueMenu& base_menu,
        int mip_r, int mip_g, int mip_b, int mip_h, int mip_s, int mip_v);

    static void make_menu_items(
        std::vector<SimpleItemValueMenu::MenuItem>& menu_items,
        int mip_r, int mip_g, int mip_b, int mip_h, int mip_s, int mip_v);

    void redraw(bool draw = true);
    bool process_key_press(int key, int key_count, bool& changed);

    int r() const { return r_; }
    int g() const { return g_; }
    int b() const { return b_; }
    int h() const { return h_; }
    int s() const { return s_; }
    int v() const { return v_; }

    void set_rgb(int r, int g, int b, bool draw = true);
    void set_hsv(int h, int s, int v, bool draw = true);

private:
    SimpleItemValueMenu& base_menu_;
    int mip_r_ = 0;
    int mip_g_ = 0;
    int mip_b_ = 0;
    int mip_h_ = 0;
    int mip_s_ = 0;
    int mip_v_ = 0;

    void set_r_value(bool draw = true);
    void set_g_value(bool draw = true);
    void set_b_value(bool draw = true);
    void set_h_value(bool draw = true);
    void set_s_value(bool draw = true);
    void set_v_value(bool draw = true);
    void transfer_rgb_to_hsv(bool draw = true);
    void transfer_hsv_to_rgb(bool draw = true);

    int r_ = 0;
    int g_ = 0;
    int b_ = 0;
    int h_ = 0;
    int s_ = 0;
    int v_ = 0;
};

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
    inline void flush() const {
        uint32_t stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm_);
        hard_assert(program_activated_);
        pio_->fdebug = stall_mask;
        busy_wait_us(1);
        while (!(pio_->fdebug & stall_mask)) {
            busy_wait_us(1);
        }
        busy_wait_us(50); // + 24000000 / baud_rate_);
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
