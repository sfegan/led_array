#include <algorithm>
#include <cmath>

#include <hardware/adc.h>

#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/input_menu.hpp"
#include "../common/popup_menu.hpp"
#include "../common/color_led.hpp"

#include "main.hpp"
#include "mono_color_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

MonoColorMenu::MonoColorMenu(SerialPIO& pio, SavedStateManager* saved_state_manager):
    SimpleItemValueMenu(make_menu_items(), "Mono color menu"),
    pio_(pio), saved_state_manager_(saved_state_manager),
    c_(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V)
{
    timer_interval_us_ = 1000000; // 1Hz
    c_.redraw(false);
}

void MonoColorMenu::send_color_string()
{
    // puts("Sending color string .....");
    uint32_t color_code = rgb_to_grbz(c_.r(), c_.g(), c_.b());
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

    RGBHSVMenuItems::make_menu_items(menu_items, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V);

    menu_items.at(MIP_Z)           = {"z       : Zero all color components (black)", 0, ""};
    menu_items.at(MIP_W)           = {"W       : Max all color components (white)", 0, ""};

    menu_items.at(MIP_WRITE_STATE) = {"Ctrl-w  : Write state to flash", 0, ""};
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
    bool changed = false;
    if(c_.process_key_press(key, key_count, changed)) {
        if(changed) {
            send_color_string();
        }
        return true;
    }

    switch(key) {
    case 'z':
    case 'Z':
        c_.set_rgb(0, 0, 0);
        send_color_string();
        break;
    case 'W':
        c_.set_rgb(255, 255, 255);
        send_color_string();
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

std::vector<int32_t> MonoColorMenu::get_saved_state()
{
    std::vector<int32_t> state;
    state.push_back(c_.r());
    state.push_back(c_.g());
    state.push_back(c_.b());
    return state;
}

bool MonoColorMenu::set_saved_state(const std::vector<int32_t>& state)
{
    if(state.size() != 3) {
        return false;
    }
    c_.set_rgb(state[0], state[1], state[2], false);
    return true;
}

int32_t MonoColorMenu::get_version()
{
    return 0;
}

int32_t MonoColorMenu::get_supplier_id()
{
    return 0x4d434f4d; // "MOCM"
}
