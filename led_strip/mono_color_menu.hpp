#pragma once

#include <vector>

#include <pico/stdlib.h>

#include "../common/menu.hpp"
#include "../common/color_led.hpp"

class MonoColorMenu: public SimpleItemValueMenu {
public:
    MonoColorMenu(SerialPIO& pio_);
    virtual ~MonoColorMenu() { }
    bool event_loop_starting(int& return_code) final;
    void event_loop_finishing(int& return_code) final;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters, absolute_time_t& next_timer) final;
    bool process_timer(bool controller_is_connected, int& return_code, absolute_time_t& next_timer) final;

private:
    enum MenuItemPositions {
        MIP_NLED,
        MIP_NON,
        MIP_FRONT_BACK,
        MIP_R,
        MIP_G,
        MIP_B,
        MIP_H,
        MIP_S,
        MIP_V,
        MIP_Z,
        MIP_W,
        MIP_EXIT,
        MIP_NUM_ITEMS // MUST BE LAST ITEM IN LIST
    };

    std::vector<MenuItem> make_menu_items();

    void set_nled_value(bool draw = true);
    void set_non_value(bool draw = true);
    void set_front_back_value(bool draw = true);

    void set_r_value(bool draw = true);
    void set_g_value(bool draw = true);
    void set_b_value(bool draw = true);

    void set_h_value(bool draw = true);
    void set_s_value(bool draw = true);
    void set_v_value(bool draw = true); 

    void send_color_string();
    void transfer_rgb_to_hsv(bool draw = true);
    void transfer_hsv_to_rgb(bool draw = true);    

    SerialPIO& pio_;

    int nled_ = 0;
    int non_ = 0;
    bool back_ = false;
    int r_ = 0;
    int g_ = 0;
    int b_ = 0;
    int h_ = 0;
    int s_ = 0;
    int v_ = 0;
};
