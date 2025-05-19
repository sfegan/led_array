#include <algorithm>
#include <cmath>

#include <hardware/adc.h>

#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/input_menu.hpp"
#include "../common/color_led.hpp"
#include "main.hpp"
#include "bi_color_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

BiColorMenu::BiColorMenu(SerialPIO& pio):
    SimpleItemValueMenu(make_menu_items(), "Bi color menu"),
    pio_(pio), 
    c0_(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
    c1_(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V)
{
    timer_interval_us_ = 1000000; // 1Hz
    c0_.redraw(false);
}

uint32_t BiColorMenu::color_code(int iled) 
{
    iled %= period_;
    float fperiod = float(period_);
    float xx = 100.0f / (fperiod * (50 - hold_));
    float fled = float(iled);
    float x = std::min(fled * xx, 1.0f - (fled - 0.5f*fperiod) * xx);
    x = std::max(x, 0.0f);
    x = std::min(x, 1.0f);
    float r = float(c0_.r())*x + float(c1_.r())*(1-x);
    float g = float(c0_.g())*x + float(c1_.g())*(1-x);
    float b = float(c0_.b())*x + float(c1_.b())*(1-x);
    uint32_t color_code = rgb_to_grbz(r, g, b);
    return color_code;
}

void BiColorMenu::send_color_string()
{
    // puts("Sending color string .....");
    if(pio_.back()) {
        pio_.put_pixel(0, pio_.nled()-pio_.non());
        for(int iled=pio_.non(); iled>0; ) {
            pio_.put_pixel(color_code(--iled), 1);
        }
    } else {
        for(int iled=0; iled<pio_.non(); iled++) {
            pio_.put_pixel(color_code(iled), 1);
        }
        pio_.put_pixel(0, pio_.nled()-pio_.non());
    }
    pio_.flush();
    // puts("..... color string sent");
}

std::vector<SimpleItemValueMenu::MenuItem> BiColorMenu::make_menu_items() 
{
    std::vector<SimpleItemValueMenu::MenuItem> menu_items(MIP_NUM_ITEMS);

    menu_items.at(MIP_SWITCH)      = {"/       : Switch color set", 1, "0"};

    RGBHSVMenuItems::make_menu_items(menu_items, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V);

    menu_items.at(MIP_PERIOD)      = {"-/p/+   : Set transition period in LEDs", 5, "20"};
    menu_items.at(MIP_HOLD)        = {"[/m/]   : Set maibtain percentage", 3, "0"};

    menu_items.at(MIP_EXIT)        = {"q       : Exit menu", 0, ""};

    return menu_items;
}

void BiColorMenu::set_cset_value(bool draw)
{
    menu_items_[MIP_SWITCH].value = std::to_string(cset_);
    if(draw)draw_item_value(MIP_SWITCH);
}

void BiColorMenu::set_period_value(bool draw)
{
    menu_items_[MIP_PERIOD].value = std::to_string(period_);
    if(draw)draw_item_value(MIP_PERIOD);
}

void BiColorMenu::set_hold_value(bool draw)
{
    menu_items_[MIP_HOLD].value = std::to_string(hold_);
    if(draw)draw_item_value(MIP_HOLD);
}

bool BiColorMenu::event_loop_starting(int& return_code)
{
    pio_.activate_program();
    send_color_string();
    return true;
}

void BiColorMenu::event_loop_finishing(int& return_code)
{
    pio_.put_pixel(0, pio_.nled());
    pio_.flush();
    pio_.deactivate_program();
}

bool BiColorMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    bool changed = false;

    RGBHSVMenuItems* c = cset_ == 0 ? &c0_ : &c1_;
    RGBHSVMenuItems* c_alt = cset_ == 0 ? &c1_ : &c0_;

    if(c->process_key_press(key, key_count, changed)) {
        if(changed) {
            send_color_string();
        }
        return true;
    }

    switch(key) {
    case '/':
        cset_ = 1 - cset_;
        set_cset_value();
        c_alt->redraw(true);
        break;

    case '+':
        if(increase_value_in_range(period_, 2*pio_.non(), (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_period_value();
            send_color_string();
        }
        break;
    case '-':
        if(decrease_value_in_range(period_, 2, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_period_value();
            send_color_string();
        }
        break; 
    case 'p':
    case 'P':
        if(InplaceInputMenu::input_value_in_range(period_, 2, 2*pio_.non(), this, MIP_PERIOD, 5)) {
            send_color_string();
        }
        set_period_value();
        break;

    case ']':
        if(increase_value_in_range(hold_, 49, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_hold_value();
            send_color_string();
        }
        break;
    case '[':
        if(decrease_value_in_range(hold_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_hold_value();
            send_color_string();
        }
        break; 
    case 'm':
    case 'M':
        if(InplaceInputMenu::input_value_in_range(hold_, 0, 49, this, MIP_HOLD, 3)) {
            send_color_string();
        }
        set_hold_value();
        break;

    case 'q':
    case 'Q':
        return_code = 0;
        return false;

    case 'D':
        for(int iled=0; iled<pio_.non(); iled++) {
            std::string s = std::to_string(iled) + ": " + std::to_string(color_code(iled));
            puts(s.c_str());
        }
        break;

    default:
        if(key_count==1) {
            beep();
        }
    }

    return true;
}

bool BiColorMenu::process_timer(bool controller_is_connected, int& return_code, 
    absolute_time_t& next_timer)
{
    set_heartbeat(!heartbeat_);
    return true;
}
