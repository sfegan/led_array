#include <algorithm>
#include <cmath>
#include <random>

#include <hardware/adc.h>

#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/input_menu.hpp"
#include "../common/popup_menu.hpp"
#include "../common/color_led.hpp"

#include "main.hpp"
#include "spider_run_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

SpiderRunMenu::SpiderRunMenu(SerialPIO& pio, SavedStateManager* saved_state_manager):
    SimpleItemValueMenu(make_menu_items(), "Spider run menu"),
    pio_(pio), saved_state_manager_(saved_state_manager),
    c_(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
    rng_(12939) // Essential supply
{
    timer_interval_us_ = 20000; // 50Hz
    c_.redraw(false);
}

void SpiderRunMenu::send_color_string()
{
    // puts("Sending color string .....");
    uint32_t cc = rgb_to_grbz(c_.r(), c_.g(), c_.b());
    for(int i=0;i<pio_.non(); ++i) {
        color_codes_[i] = cc;
    }
    cc = 0;
    for(const auto& s : spiders_) {
        color_codes_[s.x0] = cc;
        color_codes_[s.x1] = cc;
    }

    if(pio_.back()) {
        pio_.put_pixel(0, pio_.nled()-pio_.non());
        pio_.put_pixel_vector(color_codes_);
    } else {
        pio_.put_pixel_vector(color_codes_);
        pio_.put_pixel(0, pio_.nled()-pio_.non());
    }
    pio_.flush();
    // puts("..... color string sent");
}

std::vector<SimpleItemValueMenu::MenuItem> SpiderRunMenu::make_menu_items() 
{
    std::vector<SimpleItemValueMenu::MenuItem> menu_items(MIP_NUM_ITEMS);

    RGBHSVMenuItems::make_menu_items(menu_items, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V);

    menu_items.at(MIP_SPAWN_RATE)  = {"</^/>     : Decrease/Set/Increase spawn rate (0..255)", 3, "20"};
    menu_items.at(MIP_MAX_TUPDATE)  = {"[/]     : Decrease/Increase maximum spider speed (min..127)", 3, "10"};
    menu_items.at(MIP_MIN_TUPDATE)  = {"{/}     : Decrease/Increase minimum spider speed (1..max)", 3, "10"};
    menu_items.at(MIP_COLLISION)   = {"c       : Enable collision detection", 4, "OFF"};

    menu_items.at(MIP_WRITE_STATE) = {"Ctrl-w  : Write state to flash", 0, ""};
    menu_items.at(MIP_EXIT)        = {"q       : Exit menu", 0, ""};

    return menu_items;
}

void SpiderRunMenu::set_spawn_value(bool draw)
{
    menu_items_[MIP_SPAWN_RATE].value = std::to_string(spawn_rate_);
    if(draw)draw_item_value(MIP_SPAWN_RATE);
}

void SpiderRunMenu::set_max_tupdate_value(bool draw)
{
    menu_items_[MIP_MAX_TUPDATE].value = std::to_string(max_tupdate_);
    if(draw)draw_item_value(MIP_MAX_TUPDATE);
}

void SpiderRunMenu::set_min_tupdate_value(bool draw)
{
    menu_items_[MIP_MIN_TUPDATE].value = std::to_string(min_tupdate_);
    if(draw)draw_item_value(MIP_MIN_TUPDATE);
}

void SpiderRunMenu::set_collision_value(bool draw)
{
    menu_items_[MIP_COLLISION].set_onoff(collision_);
    if(draw)draw_item_value(MIP_COLLISION);
}

bool SpiderRunMenu::event_loop_starting(int& return_code)
{
    color_codes_.resize(pio_.non());
    pio_.activate_program();
    send_color_string();
    return true;
}

void SpiderRunMenu::event_loop_finishing(int& return_code)
{
    pio_.put_pixel(0, pio_.nled());
    pio_.flush();
    pio_.deactivate_program();
}

bool SpiderRunMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    bool changed = false;
    if(c_.process_key_press(key, key_count, changed)) {
        if(changed) {
            send_color_string();
        }
        return true;
    }

    switch(key) {
    case '>':
        if(increase_value_in_range(spawn_rate_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_spawn_value();
        }
        break;
    case '<':
        if(decrease_value_in_range(spawn_rate_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_spawn_value();
        }
        break;
    case '^':
        InplaceInputMenu::input_value_in_range(spawn_rate_, 0, 255, this, MIP_SPAWN_RATE, 3);
        set_spawn_value();
        break;

    case '}':
        if(increase_value_in_range(min_tupdate_, 127, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_min_tupdate_value();
            if(min_tupdate_ > max_tupdate_) {
                max_tupdate_ = min_tupdate_;
                set_max_tupdate_value();
            }
        }
        break;
    case '{':
        if(decrease_value_in_range(min_tupdate_, 1, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_min_tupdate_value();
        }
        break;

    case ']':
        if(increase_value_in_range(max_tupdate_, 127, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_max_tupdate_value();
        }
        break;
    case '[':
        if(decrease_value_in_range(max_tupdate_, 1, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_max_tupdate_value();
            if(max_tupdate_ < min_tupdate_) {
                min_tupdate_ = max_tupdate_;
                set_min_tupdate_value();
            }
        }
        break;

    case 'q':
    case 'Q':
        return_code = 0;
        return false;

    case 23:
        if(saved_state_manager_) {
            saved_state_manager_->save_state();
            PopupMenu pm("State written to flash", 2, true, this, "Information");
            pm.event_loop();
            this->redraw();
        }
        break;

    case 'D':
        printf("t = %d\n", t_);
        printf("nspider = %d\n", spiders_.size());
        for(const auto& s : spiders_) {
            printf("- x0 = %d  x1 = %d  xdest = %d  tupdate = %d  age = %d\n", s.x0, s.x1, s.xdest, s.tupdate, t_-s.t0);
        }
        break;

    default:
        if(key_count==1) {
            beep();
        }
    }

    return true;
}

bool SpiderRunMenu::process_timer(bool controller_is_connected, int& return_code, 
    absolute_time_t& next_timer)
{
    heartbeat_timer_count_ += 1;
    if(heartbeat_timer_count_ == 50) {
        if(controller_is_connected) {
            set_heartbeat(!heartbeat_);
        }
        heartbeat_timer_count_ = 0;
    }

    for(auto is = spiders_.begin(); is != spiders_.end();) {
        if((t_ - is->t0) % is->tupdate == 0) {
            if(is->x0 != is->x1) {
                // Finish half-step
                is->x1 = is->x0;
            } else if(is->x0 > is->xdest) {
                // Start half-step to "left"
                is->x0 -= 1;
            } else if(is->x0 < is->xdest) {
                // Start half-step to "right"
                is->x0 += 1;
            } else {
                // Reached destination - erase spider and advance iterator
                is = spiders_.erase(is);
                continue;
            }
        }
        is++;
    } 

    unsigned ix = rng_();
    while((ix&0xFFF) < unsigned(spawn_rate_) and spiders_.size() < (unsigned)pio_.non()/2) {
        Spider s;
        s.x0 = s.x1 = rng_() % pio_.non();
        s.xdest = rng_() % pio_.non();
        s.tupdate = min_tupdate_;
        if(max_tupdate_ > min_tupdate_) {
            s.tupdate += rng_() % (max_tupdate_ - min_tupdate_ + 1);
        }
        s.t0 = t_;
        spiders_.push_back(s);
        ix = rng_();
    }

    send_color_string();

    t_++;

    return true;
}

std::vector<int32_t> SpiderRunMenu::get_saved_state()
{
    std::vector<int32_t> state;
    state.push_back(c_.r());
    state.push_back(c_.g());
    state.push_back(c_.b());
    state.push_back(spawn_rate_);
    state.push_back(max_tupdate_);
    state.push_back(min_tupdate_);
    state.push_back(collision_);
    return state;
}

bool SpiderRunMenu::set_saved_state(const std::vector<int32_t>& state)
{
    if(state.size() != 7) {
        return false;
    }
    c_.set_rgb(state[0], state[1], state[2], false);
    spawn_rate_ = state[3];
    max_tupdate_ = state[4];
    min_tupdate_ = state[5];
    collision_ = state[6];
    return true;
}

int32_t SpiderRunMenu::get_version()
{
    return 0;
}

int32_t SpiderRunMenu::get_supplier_id()
{
    return 0x52445053; // "SPDR"
}
