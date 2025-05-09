#include <algorithm>
#include <cmath>

#include <hardware/adc.h>

#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/input_menu.hpp"
#include "main.hpp"
#include "mono_color_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

MonoColorMenu::MonoColorMenu() : 
    SimpleItemValueMenu(make_menu_items(), "Mono color menu") 
{
    // nothing to see here
    timer_interval_us_ = 10000; // 100Hz
}

void MonoColorMenu::set_nled_value(bool draw) 
{ 
    menu_items_[MIP_NLED].value = std::to_string(nled_); 
    if(draw)draw_item_value(MIP_NLED);
}

void MonoColorMenu::set_non_value(bool draw) 
{ 
    menu_items_[MIP_NON].value = std::to_string(non_); 
    if(draw)draw_item_value(MIP_NON);
}

void MonoColorMenu::set_front_back_value(bool draw) 
{ 
    menu_items_[MIP_FRONT_BACK].value = back_ ? "BACK" : "FRONT"; 
    if(draw)draw_item_value(MIP_FRONT_BACK);
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
    float r = r_ / 255.0f;
    float g = g_ / 255.0f;
    float b = b_ / 255.0f;

    float max_val = std::max({r, g, b});
    float min_val = std::min({r, g, b});
    float delta = max_val - min_val;

    if (delta == 0) {
        h_ = 0;
        s_ = 0;
        v_ = static_cast<int>(max_val * 255);

        set_h_value(draw);
        set_s_value(draw);
        set_v_value(draw);

        return;
    }

    if (max_val == r) {
        h_ = static_cast<int>(60 * fmod((g - b) / delta, 6));
    } else if (max_val == g) {
        h_ = static_cast<int>(60 * ((b - r) / delta + 2));
    } else {
        h_ = static_cast<int>(60 * ((r - g) / delta + 4));
    }

    if (h_ < 0) h_ += 360;

    s_ = static_cast<int>((delta / max_val) * 255);
    v_ = static_cast<int>(max_val * 255);

    set_h_value(draw);
    set_s_value(draw);
    set_v_value(draw);
}

void MonoColorMenu::transfer_hsv_to_rgb(bool draw) 
{
    float r, g, b;
    float h = h_ / 360.0f;
    float s = s_ / 255.0f;
    float v = v_ / 255.0f;

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

    r_ = static_cast<int>(r * 255);
    g_ = static_cast<int>(g * 255);
    b_ = static_cast<int>(b * 255);

    set_r_value(draw);
    set_g_value(draw);
    set_b_value(draw);
}

void MonoColorMenu::send_color_string()
{
}

std::vector<SimpleItemValueMenu::MenuItem> MonoColorMenu::make_menu_items() 
{
    std::vector<SimpleItemValueMenu::MenuItem> menu_items(MIP_NUM_ITEMS);
    menu_items.at(MIP_NLED)        = {"+/N/-   : Decrease/Set/Increase number of LEDs in strip", 4, "0"};
    menu_items.at(MIP_NON)         = {"</n/>   : Decrease/Set/Increase number of illuminated LEDs", 4, "0"};
    menu_items.at(MIP_FRONT_BACK)  = {"f       : Toggle between front and back", 5, "FRONT"};

    menu_items.at(MIP_R)           = {"r/1/R   : Decrease/Set/Increase red", 3, "0"};
    menu_items.at(MIP_G)           = {"g/2/G   : Decrease/Set/Increase green", 3, "0"};
    menu_items.at(MIP_B)           = {"b/3/B   : Decrease/Set/Increase blue", 3, "0"};

    menu_items.at(MIP_H)           = {"h/4/H   : Decrease/Set/Increase hue", 3, "0"};
    menu_items.at(MIP_S)           = {"s/5/S   : Decrease/Set/Increase saturation", 3, "0"};
    menu_items.at(MIP_V)           = {"v/6/V   : Decrease/Set/Increase intensity", 3, "0"};

    menu_items.at(MIP_EXIT)        = {"q       : Exit menu", 0, ""};
    return menu_items;
}

bool MonoColorMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    switch(key) {
    case '+':
        if(increase_value_in_range(nled_, MAX_PIXELS, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_nled_value();
            send_color_string();
        }
        break;
    case '-':
        if(decrease_value_in_range(nled_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_nled_value();
            if(non_ > nled_) {
                non_ = nled_;
                set_non_value();
            }
            send_color_string();
        }
        break;
    case 'N':
        if(InplaceInputMenu::input_value_in_range(nled_, 0, MAX_PIXELS, this, MIP_NLED, 4)) {
            if(non_ > nled_) {
                non_ = nled_;
                set_non_value();
            }
            send_color_string();
        }
        set_nled_value();
        break;

    case '>':
        if(increase_value_in_range(non_, nled_, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_non_value();
            send_color_string();
        }
        break;
    case '<':
        if(decrease_value_in_range(non_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_non_value();
            send_color_string();
        }
        break;
    case 'n':
        if(InplaceInputMenu::input_value_in_range(non_, nled_, MAX_PIXELS, this, MIP_NON, 4)) {
            send_color_string();
        }
        set_non_value();
        break;

    case 'F':
    case 'f':
        back_ = !back_;
        set_front_back_value();
        send_color_string();
        break;   
        
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
    heartbeat_timer_count_ += 1;
    if(heartbeat_timer_count_ == 100) {
        if(controller_is_connected) {
            set_heartbeat(!heartbeat_);
        }
        heartbeat_timer_count_ = 0;
    }

    return true;
}
