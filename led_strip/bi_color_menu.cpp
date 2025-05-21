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
    c1_(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
    rng_(123)
{
    timer_interval_us_ = 50000; // 20Hz
    c0_.redraw(false);
    update_calculations();
}

void BiColorMenu::update_calculations()
{
    int p = period_;
    if (p <= 0) p = 1; // avoid division by zero

    // Convert hold_ and balance_ to 0..65535
    int phase_frac = phase_ << (FRAC_BITS - 16);
    int hold_frac = hold_ << (FRAC_BITS - 8);
    int balance_frac = balance_ << (FRAC_BITS - 7);

    // Calculate region lengths (scaled by 65536)
    p_len_ = p << FRAC_BITS;
    int hold_len = p * hold_frac;
    trans_len_ = (p_len_ - 2 * hold_len) / 2;
    if (trans_len_ < 0) trans_len_ = 0;

    // Compute offset for balance
    int dhold_len = int64_t(hold_len) * int64_t(balance_frac) >> FRAC_BITS;

    // Apply phase
    int phase_offset = phase_frac * p;

    // Calculate the four region boundaries
    up_start_    = (phase_offset + p_len_) % p_len_;
    up_end_      = (up_start_ + trans_len_) % p_len_;
    c1_hold_end_ = (up_end_ + hold_len + dhold_len + p_len_) % p_len_;
    down_end_    = (c1_hold_end_ + trans_len_) % p_len_;

    uint64_t fp1 = (1<<31) - flash_prob_;
    uint64_t fpn = (1<<31);
    for(int i=0; i<pio_.non(); i++) {
        fpn = (fpn * fp1)>>31;
    }
    non_flash_prob_ = fpn;
}

void BiColorMenu::generate_random_flashes()
{
    for(int i=0; i<pio_.non(); i++) {
        flash_value_[i] >>= 1;
    }
    uint32_t x = rng_();
    while(x < non_flash_prob_) {
        int iled = rng_() % pio_.non();
        flash_value_[iled] = 255;
        x = rng_();
    }
}

uint32_t BiColorMenu::color_code(int iled, bool debug)
{
    // Map iled into the period
    int idx = (iled<<FRAC_BITS) % p_len_;

    int r, g, b;

    if ((up_start_ <= up_end_ && idx >= up_start_ && idx < up_end_) ||
               (up_start_ > up_end_ && (idx >= up_start_ || idx < up_end_))) {
        // c0 -> c1 (blend)
        int rel = (idx - up_start_ + p_len_) % p_len_;
        int t_fixed = (int64_t(rel)<<FRAC_BITS) / int64_t(trans_len_);
        // printf("%3d: %d %d %d\n", iled, idx, rel, t_fixed);

        r = (c0_.r() * (FRAC_ONE - t_fixed) + c1_.r() * t_fixed) >> FRAC_BITS;
        g = (c0_.g() * (FRAC_ONE - t_fixed) + c1_.g() * t_fixed) >> FRAC_BITS;
        b = (c0_.b() * (FRAC_ONE - t_fixed) + c1_.b() * t_fixed) >> FRAC_BITS;
    } else if ((up_end_ <= c1_hold_end_ && idx >= up_end_ && idx < c1_hold_end_) ||
               (up_end_ > c1_hold_end_ && (idx >= up_end_ || idx < c1_hold_end_))) {
        // hold at c1 (saturation)
        r = c1_.r();
        g = c1_.g();
        b = c1_.b();
    } else if ((c1_hold_end_ <= down_end_ && idx >= c1_hold_end_ && idx < down_end_) ||
               (c1_hold_end_ > down_end_ && (idx >= c1_hold_end_ || idx < down_end_))) {
        // c1 -> c0 (blend)
        int rel = (idx - c1_hold_end_ + p_len_) % p_len_;
        int t_fixed = FRAC_ONE - (int64_t(rel)<<FRAC_BITS) / int64_t(trans_len_);
        r = (c0_.r() * (FRAC_ONE - t_fixed) + c1_.r() * t_fixed) >> FRAC_BITS;
        g = (c0_.g() * (FRAC_ONE - t_fixed) + c1_.g() * t_fixed) >> FRAC_BITS;
        b = (c0_.b() * (FRAC_ONE - t_fixed) + c1_.b() * t_fixed) >> FRAC_BITS;
    } else {
        // hold at c0 (saturation)
        r = c0_.r();
        g = c0_.g();
        b = c0_.b();
    }

    if(flash_value_[iled] > 0) {
        r = std::max(r, flash_value_[iled]);
        g = std::max(g, flash_value_[iled]);
        b = std::max(b, flash_value_[iled]);
    }

    if(debug) {
        printf("%3d: %3d %3d %3d\n", iled, r, g, b);
    }

    return rgb_to_grbz(r, g, b);
}

void BiColorMenu::send_color_string(bool flash)
{
    // puts("Sending color string .....");

    if(flash) {
        generate_random_flashes();
    }

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

    menu_items.at(MIP_PERIOD)      = {"-/p/+   : Decrease/Set/Increase transition period in LEDs", 5, "20"};
    menu_items.at(MIP_HOLD)        = {"[/m/]   : Decrease/Set/Increase maintain length (0..127)", 3, "0"};
    menu_items.at(MIP_BALANCE)     = {"</w/>   : Decrease/Set/Increase balance (-128..128)", 4, "0"};
    menu_items.at(MIP_SPEED)       = {"Left/Right : Decrease/Increase speed", 3, "0"};
    menu_items.at(MIP_FLASH_PROB)  = {"Down/Up    : Decrease/Increase flash probability", 3, "0"};

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

void BiColorMenu::set_flash_prob_value(bool draw)
{
    menu_items_[MIP_FLASH_PROB].value = std::to_string(flash_prob_);
    if(draw)draw_item_value(MIP_FLASH_PROB);
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
            update_calculations();
            send_color_string();
        }
        break;
    case '-':
        if(decrease_value_in_range(period_, 2, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_period_value();
            update_calculations();
            send_color_string();
        }
        break; 
    case 'p':
    case 'P':
        if(InplaceInputMenu::input_value_in_range(period_, 2, 2*pio_.non(), this, MIP_PERIOD, 5)) {
            update_calculations();
            send_color_string();
        }
        set_period_value();
        break;

    case ']':
        if(increase_value_in_range(hold_, 127, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_hold_value();
            update_calculations();
            send_color_string();
        }
        break;
    case '[':
        if(decrease_value_in_range(hold_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_hold_value();
            update_calculations();
            send_color_string();
        }
        break; 
    case 'm':
    case 'M':
        if(InplaceInputMenu::input_value_in_range(hold_, 0, 127, this, MIP_HOLD, 3)) {
            update_calculations();
            send_color_string();
        }
        set_hold_value();
        break;

    case '>':
        if(increase_value_in_range(balance_, 128, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_balance_value();
            update_calculations();
            send_color_string();
        }
        break;
    case '<':
        if(decrease_value_in_range(balance_, -128, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_balance_value();
            update_calculations();
            send_color_string();
        }
        break; 
    case 'w':
    case 'W':
        if(InplaceInputMenu::input_value_in_range(balance_, -128, 128, this, MIP_BALANCE, 4)) {
            update_calculations();
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

    case KEY_UP:
        if(increase_value_in_range(flash_prob_, 256, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_flash_prob_value();
        }
        break;
    case KEY_DOWN:
        if(decrease_value_in_range(flash_prob_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_flash_prob_value();
        }
        break;
    case '0':
        if(flash_prob_ != 0) {
            flash_prob_ = 0;
            set_flash_prob_value();
        }
        break;

    case 'q':
    case 'Q':
        return_code = 0;
        return false;

    case 'D':
        for(int iled=0; iled<pio_.non(); iled++) {
            uint32_t cc  = color_code(iled, true);
        }
        printf("p_len = %d\n", p_len_);
        printf("trans_len = %d\n", trans_len_);
        printf("up_start = %d\n", up_start_);
        printf("up_end = %d\n", up_end_);
        printf("c1_hold_end = %d\n", c1_hold_end_);
        printf("down_end = %d\n", down_end_);   
        printf("non_flash_prob = %d\n", non_flash_prob_);
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

    if(speed_ != 0 and flash_prob_ != 0) {
        phase_ = (phase_ + (speed_<<6)) % 65536;
        update_calculations();
        send_color_string(true);
    }

    return true;
}
