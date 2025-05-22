#include <algorithm>
#include <cmath>
#include <cstdio>

#include <pico/time.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>

#include "../common/build_date.hpp"
#include "../common/input_menu.hpp"

#include "color_led.hpp"
#include "ws2812.pio.h"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

void rgb_to_hsv(int ir, int ig, int ib, int& ih, int& is, int& iv)
{
    float r = ir / 255.0f;
    float g = ig / 255.0f;
    float b = ib / 255.0f;

    float max_val = std::max({r, g, b});
    float min_val = std::min({r, g, b});
    float delta = max_val - min_val;

    if (delta == 0) {
        ih = 0;
        is = 0;
        iv = static_cast<int>(max_val * 255);
        return;
    }

    if (max_val == r) {
        ih = static_cast<int>(60 * fmod((g - b) / delta, 6));
    } else if (max_val == g) {
        ih = static_cast<int>(60 * ((b - r) / delta + 2));
    } else {
        ih = static_cast<int>(60 * ((r - g) / delta + 4));
    }

    if (ih < 0) ih += 360;

    is = static_cast<int>((delta / max_val) * 255);
    iv = static_cast<int>(max_val * 255);   
}

void hsv_to_rgb(int ih, int is, int iv, int& ir, int& ig, int& ib)
{
    float r, g, b;
    float h = ih / 360.0f;
    float s = is / 255.0f;
    float v = iv / 255.0f;

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

    ir = static_cast<int>(r * 255);
    ig = static_cast<int>(g * 255);
    ib = static_cast<int>(b * 255);
}

RGBHSVMenuItems::RGBHSVMenuItems(SimpleItemValueMenu& base_menu,
    int mip_r, int mip_g, int mip_b, int mip_h, int mip_s, int mip_v):
    base_menu_(base_menu), mip_r_(mip_r), mip_g_(mip_g), mip_b_(mip_b),
    mip_h_(mip_h), mip_s_(mip_s), mip_v_(mip_v)
{
    // nothing to see here
}

void RGBHSVMenuItems::make_menu_items(
    std::vector<SimpleItemValueMenu::MenuItem>& menu_items,
    int mip_r, int mip_g, int mip_b, int mip_h, int mip_s, int mip_v)
{
    menu_items.at(mip_r) = {"r/1/R   : Decrease/Set/Increase red", 3, "0"};
    menu_items.at(mip_g) = {"g/2/G   : Decrease/Set/Increase green", 3, "0"};
    menu_items.at(mip_b) = {"b/3/B   : Decrease/Set/Increase blue", 3, "0"};

    menu_items.at(mip_h) = {"h/4/H   : Decrease/Set/Increase hue", 3, "0"};
    menu_items.at(mip_s) = {"s/5/S   : Decrease/Set/Increase saturation", 3, "0"};
    menu_items.at(mip_v) = {"v/6/V   : Decrease/Set/Increase intensity", 3, "0"};
}

void RGBHSVMenuItems::redraw(bool draw)
{
    set_r_value(draw);
    set_g_value(draw);
    set_b_value(draw);
    set_h_value(draw);
    set_s_value(draw);
    set_v_value(draw);
}

void RGBHSVMenuItems::set_rgb(int r, int g, int b, bool draw)
{
    r_ = r;
    g_ = g;
    b_ = b;
    set_r_value(draw);
    set_g_value(draw);
    set_b_value(draw);
    transfer_rgb_to_hsv(draw);
}

void RGBHSVMenuItems::set_hsv(int h, int s, int v, bool draw)
{
    h_ = h;
    s_ = s;
    v_ = v;
    set_h_value(draw);
    set_s_value(draw);
    set_v_value(draw);
    transfer_hsv_to_rgb(draw);
}

bool RGBHSVMenuItems::process_key_press(int key, int key_count, bool& changed)
{
    changed = false;
    switch(key) {
    case 'R':
        if(Menu::increase_value_in_range(r_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_r_value();
            transfer_rgb_to_hsv();
            changed = true;
        }
        return true;
    case 'r':
        if(Menu::decrease_value_in_range(r_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_r_value();
            transfer_rgb_to_hsv();
            changed = true;
        }
        return true;
    case '1':
        if(InplaceInputMenu::input_value_in_range(r_, 0, 255, &base_menu_, mip_r_, 3)) {
            transfer_rgb_to_hsv();
            changed = true;
        }
        set_r_value();
        return true;

    case 'G':
        if(Menu::increase_value_in_range(g_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_g_value();
            transfer_rgb_to_hsv();
            changed = true;
        }
        return true;
    case 'g':
        if(Menu::decrease_value_in_range(g_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_g_value();
            transfer_rgb_to_hsv();
            changed = true;
        }
        return true;
    case '2':
        if(InplaceInputMenu::input_value_in_range(g_, 0, 255, &base_menu_, mip_g_, 3)) {
            transfer_rgb_to_hsv();
            changed = true;
        }
        set_g_value();
        return true;

    case 'B':
        if(Menu::increase_value_in_range(b_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_b_value();
            transfer_rgb_to_hsv();
            changed = true;
        }
        return true;
    case 'b':
        if(Menu::decrease_value_in_range(b_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_b_value();
            transfer_rgb_to_hsv();
            changed = true;
        }
        return true;
    case '3':
        if(InplaceInputMenu::input_value_in_range(b_, 0, 255, &base_menu_, mip_b_, 3)) {
            transfer_rgb_to_hsv();
            changed = true;
        }
        set_b_value();
        return true;

    case 'H':
        if(Menu::increase_value_in_range(h_, 720, (key_count >= 15 ? 5 : 1), key_count==1)) {
            if(h_ >= 360) {
                h_ -= 360;
            }
            set_h_value();
            transfer_hsv_to_rgb();
            changed = true;
        }
        return true;
    case 'h':
        if(Menu::decrease_value_in_range(h_, -360, (key_count >= 15 ? 5 : 1), key_count==1)) {
            if(h_ < 0) {
                h_ += 360;
            }
            set_h_value();
            transfer_hsv_to_rgb();
            changed = true;
        }
        return true;
    case '4':
        if(InplaceInputMenu::input_value_in_range(h_, 0, 359, &base_menu_, mip_h_, 3)) {
            transfer_hsv_to_rgb();
            changed = true;
        }
        set_h_value();
        return true;

    case 'S':
        if(Menu::increase_value_in_range(s_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_s_value();
            transfer_hsv_to_rgb();
            changed = true;
        }
        return true;
    case 's':
        if(Menu::decrease_value_in_range(s_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_s_value();
            transfer_hsv_to_rgb();
            changed = true;
        }
        return true;
    case '5':
        if(InplaceInputMenu::input_value_in_range(s_, 0, 255, &base_menu_, mip_s_, 3)) {
            transfer_hsv_to_rgb();
            changed = true;
        }
        set_s_value();
        return true;

    case 'V':
        if(Menu::increase_value_in_range(v_, 255, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_v_value();
            transfer_hsv_to_rgb();
            changed = true;
        }
        return true;
    case 'v':
        if(Menu::decrease_value_in_range(v_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_v_value();
            transfer_hsv_to_rgb();
            changed = true;
        }
        return true;
    case '6':
        if(InplaceInputMenu::input_value_in_range(v_, 0, 255, &base_menu_, mip_v_, 3)) {
            transfer_hsv_to_rgb();
            changed = true;
        }
        set_v_value();
        return true;
    }
    return false;
}

void RGBHSVMenuItems::set_r_value(bool draw)
{
    base_menu_.menu_item(mip_r_).value = std::to_string(r_);
    if(draw)base_menu_.draw_item_value(mip_r_);
}

void RGBHSVMenuItems::set_g_value(bool draw)
{
    base_menu_.menu_item(mip_g_).value = std::to_string(g_);
    if(draw)base_menu_.draw_item_value(mip_g_);
}

void RGBHSVMenuItems::set_b_value(bool draw)
{
    base_menu_.menu_item(mip_b_).value = std::to_string(b_);
    if(draw)base_menu_.draw_item_value(mip_b_);
}

void RGBHSVMenuItems::set_h_value(bool draw)
{
    base_menu_.menu_item(mip_h_).value = std::to_string(h_);
    if(draw)base_menu_.draw_item_value(mip_h_);
}

void RGBHSVMenuItems::set_s_value(bool draw)
{
    base_menu_.menu_item(mip_s_).value = std::to_string(s_);
    if(draw)base_menu_.draw_item_value(mip_s_);
}

void RGBHSVMenuItems::set_v_value(bool draw)
{
    base_menu_.menu_item(mip_v_).value = std::to_string(v_);
    if(draw)base_menu_.draw_item_value(mip_v_);
}

void RGBHSVMenuItems::transfer_rgb_to_hsv(bool draw)
{
    rgb_to_hsv(r_, g_, b_, h_, s_, v_);
    set_h_value(draw);
    set_s_value(draw);
    set_v_value(draw);
}

void RGBHSVMenuItems::transfer_hsv_to_rgb(bool draw)
{
    hsv_to_rgb(h_, s_, v_, r_, g_, b_);
    set_r_value(draw);
    set_g_value(draw);
    set_b_value(draw);
}

SerialPIO::SerialPIO(int pin, int baudrate): pin_(pin), baudrate_(baudrate)
{
    // nothing to see here
}

SerialPIO::~SerialPIO()
{
    if(program_activated_) {
        deactivate_program();
    }   
}

void SerialPIO::set_pin(int pin)
{
    hard_assert(!program_activated_);
    pin_ = pin;
}

void SerialPIO::set_baudrate(int baudrate)
{
    hard_assert(!program_activated_);
    baudrate_ = baudrate;
}   

void SerialPIO::activate_program()
{
    // puts("Activating WS2812 program .....");
    hard_assert(!program_activated_);
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(
        &ws2812_program, &pio_, &sm_, &offset_, pin_, 1, true);
    pio_sm_clear_fifos(pio_, sm_);
    hard_assert(success);
    ws2812_program_init(pio_, sm_, offset_, pin_, baudrate_, false);
    program_activated_ = true;
    // puts("..... WS2812 program activated");
}

void SerialPIO::deactivate_program()
{
    // puts("Deactivating WS2812 program .....");
    hard_assert(program_activated_);
    flush();
    pio_remove_program_and_unclaim_sm(
        &ws2812_program, pio_, sm_, offset_);
    program_activated_ = false;
    // puts("..... WS2812 program deactivated");
}

SerialPIOMenu::SerialPIOMenu(int pin, int baudrate):
    SerialPIO(pin, baudrate), 
    SimpleItemValueMenu(make_menu_items())
{
    timer_interval_us_ = 50000; // 20Hz
    set_pin_value(false);
    set_baudrate_value(false);
    set_nled_value(false);
    set_non_value(false);
    set_back_value(false);
    set_lamp_test_value(false);
}

std::vector<int32_t> SerialPIOMenu::get_saved_state()
{
    std::vector<int32_t> state;
    state.push_back(pin_);
    state.push_back(baudrate_);
    state.push_back(nled_);
    state.push_back(non_);
    state.push_back(back_ ? 1 : 0);
    return state;
}

bool SerialPIOMenu::set_saved_state(const std::vector<int32_t>& state)
{
    if(state.size() != 5) {
        return false;
    }
    pin_ = state[0];
    baudrate_ = state[1];
    nled_ = state[2];
    non_ = state[3];
    back_ = (state[4] != 0);
    set_pin_value(false);
    set_baudrate_value(false);
    set_nled_value(false);
    set_non_value(false);
    set_back_value(false);
    return true;
}

int32_t SerialPIOMenu::get_version()
{
    return 0;
}
    
int32_t SerialPIOMenu::get_supplier_id()
{
    return 0x4f495053; // "SPIO"
}

SerialPIOMenu::~SerialPIOMenu()
{
    // nothing to see here
}

void SerialPIOMenu::set_pin_value(bool draw)
{
    menu_items_[MIP_PIN].value = std::to_string(pin_);
    if(draw)draw_item_value(MIP_PIN);
}

void SerialPIOMenu::set_baudrate_value(bool draw)
{
    menu_items_[MIP_BAUDRATE].value = std::to_string(baudrate_);
    if(draw)draw_item_value(MIP_BAUDRATE);
}

void SerialPIOMenu::set_nled_value(bool draw)
{
    menu_items_[MIP_NLED].value = std::to_string(nled_);
    if(draw)draw_item_value(MIP_NLED);
}

void SerialPIOMenu::set_non_value(bool draw)
{
    menu_items_[MIP_NON].value = std::to_string(non_);
    if(draw)draw_item_value(MIP_NON);
}

void SerialPIOMenu::set_back_value(bool draw)
{
    menu_items_[MIP_BACK].value = back_ ? "BACK" : "FRONT";
    if(draw)draw_item_value(MIP_BACK);
}

void SerialPIOMenu::set_lamp_test_value(bool draw)
{
    menu_items_[MIP_LAMP_TEST].value = (lamp_test_cycle_>=0) ? "<ON>" : "OFF";
    if(draw)draw_item_value(MIP_LAMP_TEST);
}

std::vector<SerialPIOMenu::MenuItem> SerialPIOMenu::make_menu_items()
{
    std::vector<MenuItem> menu_items(MIP_NUM_ITEMS);
    menu_items.at(MIP_PIN)         = {"P       : Set GPIO pin", 2, "0"};
    menu_items.at(MIP_BAUDRATE)    = {"B       : Set baud rate", 8, "0"};
    menu_items.at(MIP_NLED)        = {"+/N/-   : Decrease/Set/Increase number of LEDs", 4, "0"};
    menu_items.at(MIP_NON)         = {"</n/>   : Decrease/Set/Increase number of active LEDs", 4, "0"};
    menu_items.at(MIP_BACK)        = {"f       : Set front/back", 5, "FRONT"};
    menu_items.at(MIP_LAMP_TEST)   = {"l       : Lamp test", 4, ""};
    menu_items.at(MIP_EXIT)        = {"q       : Quit", 0, ""};
    return menu_items;
}

void SerialPIOMenu::send_color_string()
{
    // puts("Sending color string .....");
    if(lamp_test_cycle_ < 0 or non_ == 0) {
        put_pixel(0, nled_);
    } else {
        uint32_t color_code = rgb_to_grbz(0, 7, 0) >> (lamp_test_cycle_*8);
        if(back_) {
            put_pixel(0, nled_-non_);
            put_pixel(color_code, non_ - lamp_test_count_ - 1);
            put_pixel(color_code<<4, 1);
            put_pixel(color_code, lamp_test_count_);
        } else {
            put_pixel(color_code, lamp_test_count_);
            put_pixel(color_code<<4, 1);
            put_pixel(color_code, non_ - lamp_test_count_ - 1);
            put_pixel(0, nled_-non_);
        }
    }
    flush();
    // puts("..... color string sent");
}

void SerialPIOMenu::enable_lamp_test()
{
    if(lamp_test_cycle_ < 0) {
        activate_program();
        lamp_test_cycle_ = 0;
        lamp_test_count_ = 0;
        send_color_string();        
    }
}

void SerialPIOMenu::disable_lamp_test()
{
    if(lamp_test_cycle_ >= 0) {
        lamp_test_cycle_ = -1;
        lamp_test_count_ = 0;
        send_color_string();        
        deactivate_program();
    }
}

bool SerialPIOMenu::event_loop_starting(int& return_code)
{
    return true;
}

void SerialPIOMenu::event_loop_finishing(int& return_code)
{
    // nothing to see here
}

bool SerialPIOMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    switch(key) {
    case 'P':
        if(lamp_test_cycle_ >= 0) {
            if(key_count==1) {
                beep();
            }
        } else {
            InplaceInputMenu::input_value_in_range(pin_, 0, 28, this, MIP_PIN, 2);
            set_pin_value();
        }
        break;
    case 'B':
        if(lamp_test_cycle_ >= 0) {
            if(key_count==1) {
                beep();
            }
        } else {
            InplaceInputMenu::input_value_in_range(baudrate_, 0, 10000000, this, MIP_BAUDRATE, 8);
            set_baudrate_value();
        }
        break;

    case '+':
        if(Menu::increase_value_in_range(nled_, MAX_PIXELS, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_nled_value();
        }
        break;
    case '-':
        if(Menu::decrease_value_in_range(nled_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_nled_value();
            if(non_ > nled_) {
                non_ = nled_;
                set_non_value();
                if(lamp_test_cycle_ >= 0) {
                    lamp_test_count_ = std::min(lamp_test_count_, non_ - 1);
                    send_color_string();
                }
            }
        }
        break;
    case 'N':
        if(InplaceInputMenu::input_value_in_range(nled_, 0, MAX_PIXELS, this, MIP_NLED, 4)) {
            if(non_ > nled_) {
                non_ = nled_;
                set_non_value();
            }
            if(lamp_test_cycle_ >= 0) {
                lamp_test_count_ = std::min(lamp_test_count_, non_ - 1);
                send_color_string();
            }
        }
        set_nled_value();
        break;

    case '>':
        if(Menu::increase_value_in_range(non_, nled_, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_non_value();
            if(lamp_test_cycle_ >= 0) {
                send_color_string();
            }
        }
        break;
    case '<':
        if(Menu::decrease_value_in_range(non_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_non_value();
            if(lamp_test_cycle_ >= 0) {
                lamp_test_count_ = std::min(lamp_test_count_, non_ - 1);
                send_color_string();
            }
        }
        break;
    case 'n':
        if(InplaceInputMenu::input_value_in_range(non_, 0, nled_, this, MIP_NON, 4)) {
            if(lamp_test_cycle_ >= 0) {
                lamp_test_count_ = std::min(lamp_test_count_, non_ - 1);
                send_color_string();
            }
        }
        set_non_value();
        break;

    case 'F':
    case 'f':
        back_ = !back_;
        set_back_value();
        if(lamp_test_cycle_ >= 0) {
            send_color_string();
        }
        break;

    case 'L':
    case 'l':
        if(lamp_test_cycle_ < 0) {
            enable_lamp_test();
        } else {
            disable_lamp_test();
        }
        set_lamp_test_value();
        break;
        
    case 'q':
    case 'Q':
        if(lamp_test_cycle_ >= 0) {
            disable_lamp_test();
            set_lamp_test_value();
        }
        return_code = 0;
        return false;

    default:
        if(key_count==1) {
            beep();
        }
    }

    return true;   
}

bool SerialPIOMenu::process_timer(bool controller_is_connected, int& return_code,
    absolute_time_t& next_timer)
{
    heartbeat_timer_count_ += 1;
    if(heartbeat_timer_count_ == 20) {
        if(controller_is_connected) {
            set_heartbeat(!heartbeat_);
        }
        heartbeat_timer_count_ = 0;
    }

    if(lamp_test_cycle_ >= 0) {
        lamp_test_count_ += 1;
        if(lamp_test_count_ >= non_) {
            lamp_test_count_ = 0;
            lamp_test_cycle_ += 1;
            if(lamp_test_cycle_ == 3) {
                lamp_test_cycle_ = 0;
            }
        }
        send_color_string();
    }
    return true;
}
