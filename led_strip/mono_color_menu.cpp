#include <algorithm>
#include <cmath>

#include <hardware/adc.h>

#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/input_menu.hpp"
#include "../common/color_led.hpp"
#include "main.hpp"
#include "mono_color_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

MonoColorMenu::MonoColorMenu(SerialPIO& pio):
    SimpleItemValueMenu(make_menu_items(), "Mono color menu"),
    pio_(pio)
{
    timer_interval_us_ = 1000000; // 1Hz

    set_r_value(false);
    set_g_value(false);
    set_b_value(false);
    set_h_value(false);
    set_s_value(false);
    set_v_value(false);
}

void MonoColorMenu::set_r_value(bool draw) 
{ 
    menu_items_[MIP_R].value = std::to_string(r_); 
    if(draw)draw_item_value(MIP_R);
}

void MonoColorMenu::set_g_value(bool draw) 
{ 
    menu_items_[MIP_G].value = std::to_string(g_); 
    if(draw)draw_item_value(MIP_G);
}

void MonoColorMenu::set_b_value(bool draw) 
{ 
    menu_items_[MIP_B].value = std::to_string(b_); 
    if(draw)draw_item_value(MIP_B);
}

void MonoColorMenu::set_h_value(bool draw) 
{ 
    menu_items_[MIP_H].value = std::to_string(h_); 
    if(draw)draw_item_value(MIP_H);
}

void MonoColorMenu::set_s_value(bool draw) 
{ 
    menu_items_[MIP_S].value = std::to_string(s_); 
    if(draw)draw_item_value(MIP_S);
}

void MonoColorMenu::set_v_value(bool draw) 
{ 
    menu_items_[MIP_V].value = std::to_string(v_); 
    if(draw)draw_item_value(MIP_V);
}

void MonoColorMenu::transfer_rgb_to_hsv(bool draw) 
{
    rgb_to_hsv(r_, g_, b_, h_, s_, v_);
    set_h_value(draw);
    set_s_value(draw);
    set_v_value(draw);
}

void MonoColorMenu::transfer_hsv_to_rgb(bool draw) 
{
    hsv_to_rgb(h_, s_, v_, r_, g_, b_);
    set_r_value(draw);
    set_g_value(draw);
    set_b_value(draw);
}

void MonoColorMenu::send_color_string()
{
    // puts("Sending color string .....");
    uint32_t color_code = rgb_to_grbz(r_, g_, b_);
    if(pio_.back()) {
        pio_.put_pixel(0, pio_.nled()-pio_.non());
        pio_.put_pixel(color_code, pio_.non());
    } else {
        pio_.put_pixel(color_code, pio_.non());
        pio_.put_pixel(0, pio_.nled()-pio_.non());
    }
    pio_.flush();
    // puts("..... color string sent");
}

std::vector<SimpleItemValueMenu::MenuItem> MonoColorMenu::make_menu_items() 
{
    std::vector<SimpleItemValueMenu::MenuItem> menu_items(MIP_NUM_ITEMS);
    menu_items.at(MIP_R)           = {"r/1/R   : Decrease/Set/Increase red", 3, "0"};
    menu_items.at(MIP_G)           = {"g/2/G   : Decrease/Set/Increase green", 3, "0"};
    menu_items.at(MIP_B)           = {"b/3/B   : Decrease/Set/Increase blue", 3, "0"};

    menu_items.at(MIP_H)           = {"h/4/H   : Decrease/Set/Increase hue", 3, "0"};
    menu_items.at(MIP_S)           = {"s/5/S   : Decrease/Set/Increase saturation", 3, "0"};
    menu_items.at(MIP_V)           = {"v/6/V   : Decrease/Set/Increase intensity", 3, "0"};

    menu_items.at(MIP_Z)           = {"z       : Zero all color components (black)", 0, ""};
    menu_items.at(MIP_W)           = {"W       : Max all color components (white)", 0, ""};

    menu_items.at(MIP_EXIT)        = {"q       : Exit menu", 0, ""};

    return menu_items;
}

bool MonoColorMenu::event_loop_starting(int& return_code)
{
    pio_.activate_program();
    send_color_string();
    return true;
}

void MonoColorMenu::event_loop_finishing(int& return_code)
{
    pio_.put_pixel(0, pio_.nled());
    pio_.flush();
    pio_.deactivate_program();
}

bool MonoColorMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    switch(key) {
    case 'R':
        if(increase_value_in_range(r_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            send_color_string();
            set_r_value();
            transfer_rgb_to_hsv();
        }
        break;
    case 'r':
        if(decrease_value_in_range(r_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            send_color_string();
            set_r_value();
            transfer_rgb_to_hsv();
        }
        break;
    case '1':
        if(InplaceInputMenu::input_value_in_range(r_, 0, 255, this, MIP_R, 3)) {
            send_color_string();
            transfer_rgb_to_hsv();
        }
        set_r_value();
        break;
    case 'G':
        if(increase_value_in_range(g_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            send_color_string();
            set_g_value();
            transfer_rgb_to_hsv();
        }
        break;
    case 'g':
        if(decrease_value_in_range(g_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            send_color_string();
            set_g_value();
            transfer_rgb_to_hsv();
        }
        break;
    case '2':
        if(InplaceInputMenu::input_value_in_range(g_, 0, 255, this, MIP_G, 3)) {
            send_color_string();
            transfer_rgb_to_hsv();
        }
        set_g_value();
        break;
    case 'B':
        if(increase_value_in_range(b_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            send_color_string();
            set_b_value();
            transfer_rgb_to_hsv();
        }
        break;
    case 'b':
        if(decrease_value_in_range(b_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            send_color_string();
            set_b_value();
            transfer_rgb_to_hsv();
        }
        break;
    case '3':
        if(InplaceInputMenu::input_value_in_range(b_, 0, 255, this, MIP_B, 3)) {
            send_color_string();
            transfer_rgb_to_hsv();
        }
        set_b_value();
        break;

    case 'z':
    case 'Z':
        r_ = 0;
        g_ = 0;
        b_ = 0;
        h_ = 0;
        s_ = 0;
        v_ = 0;
        set_r_value();
        set_g_value();
        set_b_value();
        set_h_value();
        set_s_value();
        set_v_value();
        send_color_string();
        break;
    case 'W':
        r_ = 255;
        g_ = 255;
        b_ = 255;
        h_ = 0;
        s_ = 0;
        v_ = 255;
        set_r_value();
        set_g_value();
        set_b_value();
        set_h_value();
        set_s_value();
        set_v_value();
        send_color_string();
        break;
    
    case 'H':
        if(increase_value_in_range(h_, 720, (key_count >= 15 ? 5 : 1), key_count==1)) {
            if(h_ >= 360) {
                h_ -= 360;
            }
            set_h_value();
            transfer_hsv_to_rgb();
            send_color_string();
        }
        break;
    case 'h':
        if(decrease_value_in_range(h_, -360, (key_count >= 15 ? 5 : 1), key_count==1)) {
            if(h_ < 0) {
                h_ += 360;
            }
            set_h_value();
            transfer_hsv_to_rgb();
            send_color_string();
        }
        break;
    case '4':
        if(InplaceInputMenu::input_value_in_range(h_, 0, 255, this, MIP_H, 3)) {
            transfer_hsv_to_rgb();
            send_color_string();
        }
        set_h_value();
        break;
    case 'S':
        if(increase_value_in_range(s_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_s_value();
            transfer_hsv_to_rgb();
            send_color_string();
        }
        break;
    case 's':
        if(decrease_value_in_range(s_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_s_value();
            transfer_hsv_to_rgb();
            send_color_string();
        }
        break;
    case '5':
        if(InplaceInputMenu::input_value_in_range(s_, 0, 255, this, MIP_S, 3)) {
            transfer_hsv_to_rgb();
            send_color_string();
        }
        set_s_value();
        break;
    case 'V':
        if(increase_value_in_range(v_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_v_value();
            transfer_hsv_to_rgb();
            send_color_string();
        }
        break;
    case 'v':
        if(decrease_value_in_range(v_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_v_value();
            transfer_hsv_to_rgb();
            send_color_string();
        }
        break;
    case '6':
        if(InplaceInputMenu::input_value_in_range(v_, 0, 255, this, MIP_V, 3)) {
            transfer_hsv_to_rgb();
            send_color_string();
        }
        set_v_value();
        break;

    case 'q':
    case 'Q':
        return_code = 0;
        return false;

    default:
        if(key_count==1) {
            beep();
        }
    }

    return true;
}

bool MonoColorMenu::process_timer(bool controller_is_connected, int& return_code, 
    absolute_time_t& next_timer)
{
    set_heartbeat(!heartbeat_);
    return true;
}
