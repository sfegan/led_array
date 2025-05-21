#include <sstream>
#include <iomanip>
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
    timer_interval_us_ = 50000; // 20Hz
    c0_.redraw(false);
}

uint32_t BiColorMenu::color_code(int i_led)
{
    // period_ = total number of LEDs in the cycle
    // hold_ = percent (0-127) of period_ to hold at each color
    // balance_ = percent (0-255) of period_ to shift the center point
    // phase_ = phase offset (0 to 65535)

    int p = period_;
    if (p <= 0) p = 1; // avoid division by zero

    // All calculations in integer math, using 0..65535 for fractions
    const int FRAC_BITS = 16;
    const int FRAC_ONE = 1 << FRAC_BITS;

    // Convert hold_ and balance_ to 0..65535
    int hold_frac = hold_ << (FRAC_BITS - 8);
    int balance_frac = balance_ << (FRAC_BITS - 8);

    // Calculate region lengths
    int hold_len = (p * hold_frac + (FRAC_ONE/2)) >> FRAC_BITS;
    int trans_len = (p - 2 * hold_len) / 2;
    if (trans_len < 0) trans_len = 0;

    // Compute offset for balance
    int delta_hold_len = (hold_len * balance_frac) >> FRAC_BITS;

    // Apply phase (phase_ is 0..65535, maps to 0..p in fixed point)
    // Use 16.16 fixed point for sub-LED precision
    uint32_t p_fixed = (uint32_t)p << FRAC_BITS;
    uint32_t led_pos_fixed = ((uint32_t)i_led << FRAC_BITS);

    // phase_ is 0..65535, so multiply by p and shift down to 16.16
    uint32_t phase_offset_fixed = ((uint64_t)phase_ * p_fixed) >> 16;

    // Calculate the four region boundaries in 16.16 fixed point
    uint32_t up_start_fixed = (phase_offset_fixed + p_fixed) % p_fixed;
    uint32_t up_end_fixed = (up_start_fixed + ((uint32_t)trans_len << FRAC_BITS)) % p_fixed;
    uint32_t c1_hold_end_fixed = (up_end_fixed + ((uint32_t)(hold_len + delta_hold_len) << FRAC_BITS) + p_fixed) % p_fixed;
    uint32_t down_end_fixed = (c1_hold_end_fixed + ((uint32_t)trans_len << FRAC_BITS)) % p_fixed;
    // uint32_t c0_hold_end_fixed = (down_end_fixed + ((uint32_t)(hold_len - delta_hold_len) << FRAC_BITS) + p_fixed) % p_fixed;

    // Map i_led into the period, in 16.16 fixed point
    uint32_t idx_fixed = led_pos_fixed % p_fixed;

    int r, g, b;

    if (trans_len == 0) {
        // Degenerate case: square wave, just alternate between c0 and c1
        uint32_t c1_start_fixed = (phase_offset_fixed + p_fixed) % p_fixed;
        uint32_t c1_end_fixed = (c1_start_fixed + ((uint32_t)(hold_len + delta_hold_len) << FRAC_BITS) + p_fixed) % p_fixed;
        bool in_c1;
        if (hold_len == 0) {
            in_c1 = false;
        } else if (c1_start_fixed < c1_end_fixed) {
            in_c1 = (idx_fixed >= c1_start_fixed && idx_fixed < c1_end_fixed);
        } else {
            in_c1 = (idx_fixed >= c1_start_fixed || idx_fixed < c1_end_fixed);
        }
        if (in_c1) {
            r = c1_.r();
            g = c1_.g();
            b = c1_.b();
        } else {
            r = c0_.r();
            g = c0_.g();
            b = c0_.b();
        }
    } else if ((up_start_fixed <= up_end_fixed && idx_fixed >= up_start_fixed && idx_fixed < up_end_fixed) ||
               (up_start_fixed > up_end_fixed && (idx_fixed >= up_start_fixed || idx_fixed < up_end_fixed))) {
        // c0 -> c1 (blend)
        uint32_t rel_fixed = (idx_fixed + p_fixed - up_start_fixed) % p_fixed;
        // Use fractional LED precision for ramp
        uint32_t ramp_len_fixed = ((uint32_t)trans_len << FRAC_BITS);
        uint32_t t_fixed = (rel_fixed * FRAC_ONE + (ramp_len_fixed/2)) / ramp_len_fixed;
        if (t_fixed > FRAC_ONE) t_fixed = FRAC_ONE;
        r = (c0_.r() * (FRAC_ONE - t_fixed) + c1_.r() * t_fixed) >> FRAC_BITS;
        g = (c0_.g() * (FRAC_ONE - t_fixed) + c1_.g() * t_fixed) >> FRAC_BITS;
        b = (c0_.b() * (FRAC_ONE - t_fixed) + c1_.b() * t_fixed) >> FRAC_BITS;
    } else if ((up_end_fixed <= c1_hold_end_fixed && idx_fixed >= up_end_fixed && idx_fixed < c1_hold_end_fixed) ||
               (up_end_fixed > c1_hold_end_fixed && (idx_fixed >= up_end_fixed || idx_fixed < c1_hold_end_fixed))) {
        // hold at c1 (saturation)
        r = c1_.r();
        g = c1_.g();
        b = c1_.b();
    } else if ((c1_hold_end_fixed <= down_end_fixed && idx_fixed >= c1_hold_end_fixed && idx_fixed < down_end_fixed) ||
               (c1_hold_end_fixed > down_end_fixed && (idx_fixed >= c1_hold_end_fixed || idx_fixed < down_end_fixed))) {
        // c1 -> c0 (blend)
        uint32_t rel_fixed = (idx_fixed + p_fixed - c1_hold_end_fixed) % p_fixed;
        uint32_t ramp_len_fixed = ((uint32_t)trans_len << FRAC_BITS);
        uint32_t t_fixed = (rel_fixed * FRAC_ONE + (ramp_len_fixed/2)) / ramp_len_fixed;
        if (t_fixed > FRAC_ONE) t_fixed = FRAC_ONE;
        t_fixed = FRAC_ONE - t_fixed;
        r = (c0_.r() * (FRAC_ONE - t_fixed) + c1_.r() * t_fixed) >> FRAC_BITS;
        g = (c0_.g() * (FRAC_ONE - t_fixed) + c1_.g() * t_fixed) >> FRAC_BITS;
        b = (c0_.b() * (FRAC_ONE - t_fixed) + c1_.b() * t_fixed) >> FRAC_BITS;
    } else {
        // hold at c0 (saturation)
        r = c0_.r();
        g = c0_.g();
        b = c0_.b();
    }

    return rgb_to_grb(r, g, b);
}

void BiColorMenu::send_color_string()
{
    // puts("Sending color string .....");
    if(pio_.back()) {
        int nperiod = std::min(pio_.non(), period_);
        for(int iled=0, jled=pio_.nled(); iled<nperiod; iled++) {
            color_codes_[--jled] = color_code(iled);
        }
        for(int iled=nperiod, jled=pio_.nled()-nperiod, kled=pio_.nled(); iled<pio_.non(); iled++) {
            color_codes_[--jled] = color_codes_[--kled];
        }
    } else {
        int nperiod = std::min(pio_.non(), period_);
        for(int iled=0; iled<nperiod; iled++) {
            color_codes_[iled] = color_code(iled);
        }
        for(int iled=nperiod, jled=0; iled<pio_.non(); iled++,jled++) {
            color_codes_[iled] = color_codes_[jled];
        }
    }
    pio_.put_pixel_vector(color_codes_);
    pio_.flush();
    // puts("..... color string sent");
}

std::vector<SimpleItemValueMenu::MenuItem> BiColorMenu::make_menu_items() 
{
    std::vector<SimpleItemValueMenu::MenuItem> menu_items(MIP_NUM_ITEMS);

    menu_items.at(MIP_SWITCH)      = {"/       : Switch color set", 1, "0"};

    RGBHSVMenuItems::make_menu_items(menu_items, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V);

    menu_items.at(MIP_PERIOD)      = {"-/p/+      : Decrease/Set/Increase transition period in LEDs", 5, "20"};
    menu_items.at(MIP_HOLD)        = {"[/m/]      : Decrease/Set/Increase maintain length (0..127)", 3, "0"};
    menu_items.at(MIP_BALANCE)     = {"</w/>      : Decrease/Set/Increase balance (-128..128)", 4, "0"};
    menu_items.at(MIP_SPEED)       = {"Left/Right : Decrease/Increase speed", 5, "0"};

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

void BiColorMenu::set_balance_value(bool draw)
{
    menu_items_[MIP_BALANCE].value = std::to_string(balance_);
    if(draw)draw_item_value(MIP_BALANCE);
}

void BiColorMenu::set_speed_value(bool draw)
{
    menu_items_[MIP_SPEED].value = std::to_string(speed_);
    if(draw)draw_item_value(MIP_SPEED);
}

bool BiColorMenu::event_loop_starting(int& return_code)
{
    color_codes_.resize(pio_.nled());
    flash_value_.resize(pio_.nled());
    color_codes_.assign(pio_.nled(), 0);
    flash_value_.assign(pio_.nled(), 0);
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
        if(increase_value_in_range(hold_, 127, (key_count >= 15 ? 5 : 1), key_count==1)) {
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
        if(InplaceInputMenu::input_value_in_range(hold_, 0, 127, this, MIP_HOLD, 3)) {
            send_color_string();
        }
        set_hold_value();
        break;

    case '>':
        if(increase_value_in_range(balance_, 128, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_balance_value();
            send_color_string();
        }
        break;
    case '<':
        if(decrease_value_in_range(balance_, -128, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_balance_value();
            send_color_string();
        }
        break; 
    case 'w':
    case 'W':
        if(InplaceInputMenu::input_value_in_range(balance_, -128, 128, this, MIP_BALANCE, 4)) {
            send_color_string();
        }
        set_balance_value();
        break;

    case KEY_RIGHT:
        if(increase_value_in_range(speed_, 256, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_speed_value();
            send_color_string();
        }
        break;
    case KEY_LEFT:
        if(decrease_value_in_range(speed_, -256, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_speed_value();
            send_color_string();
        }
        break;
        
    case 'z':
    case 'Z':
        if(speed_ != 0) {
            speed_ = 0;
            set_speed_value();
        }
        break;

    case 'q':
    case 'Q':
        return_code = 0;
        return false;

    case 'D':
        for(int iled=0; iled<pio_.non(); iled++) {
            uint32_t cc  = color_code(iled);
            std::ostringstream ss;
            ss << std::setw(3) << iled << ": " 
               << std::setw(3) << ((cc>>16)&0xff) << " "
               << std::setw(3) << ((cc>>24)&0xff) << " "
               << std::setw(3) << ((cc>>8)&0xff);
            puts(ss.str().c_str());
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
    heartbeat_timer_count_ += 1;
    if(heartbeat_timer_count_ == 20) {
        if(controller_is_connected) {
            set_heartbeat(!heartbeat_);
        }
        heartbeat_timer_count_ = 0;
    }

    if(speed_ != 0) {
        phase_ = (phase_ + (speed_<<4)) % 65536;
        send_color_string();
    }

    return true;
}
