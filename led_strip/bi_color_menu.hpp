#pragma once

#include <vector>

#include <pico/stdlib.h>

#include "../common/menu.hpp"
#include "../common/color_led.hpp"

class BiColorMenu: public SimpleItemValueMenu {
public:
BiColorMenu(SerialPIO& pio_);
    virtual ~BiColorMenu() { }
    bool event_loop_starting(int& return_code) final;
    void event_loop_finishing(int& return_code) final;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters, absolute_time_t& next_timer) final;
    bool process_timer(bool controller_is_connected, int& return_code, absolute_time_t& next_timer) final;

private:
    enum MenuItemPositions {
        MIP_SWITCH,
        MIP_R,
        MIP_G,
        MIP_B,
        MIP_H,
        MIP_S,
        MIP_V,
        MIP_PERIOD,
        MIP_HOLD,
        MIP_BALANCE,
        MIP_SPEED,
        MIP_EXIT,
        MIP_NUM_ITEMS // MUST BE LAST ITEM IN LIST
    };

    std::vector<MenuItem> make_menu_items();

    void set_cset_value(bool draw = true);
    void set_period_value(bool draw = true);
    void set_hold_value(bool draw = true);
    void set_balance_value(bool draw = true);
    void set_speed_value(bool draw = true);
    
    uint32_t color_code(int iled, bool debug = false);
    void send_color_string();

    SerialPIO& pio_;
    RGBHSVMenuItems c0_;
    RGBHSVMenuItems c1_;

    int cset_ = 0;
    int period_ = 20;
    int hold_ = 0;
    int balance_ = 0;
    int speed_ = 0;
    int phase_ = 0;

    int heartbeat_timer_count_ = 0;
    std::vector<uint32_t> color_codes_;
    std::vector<uint32_t> flash_value_;
};
