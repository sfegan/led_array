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
    while(!all_pixel_data_sent()) {
        busy_wait_us(1);
    }
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
        if(increase_value_in_range(nled_, MAX_PIXELS, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_nled_value();
        }
        break;
    case '-':
        if(decrease_value_in_range(nled_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
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
        if(increase_value_in_range(non_, nled_, (key_count >= 15 ? 5 : 1), key_count==1)) {
            set_non_value();
            if(lamp_test_cycle_ >= 0) {
                send_color_string();
            }
        }
        break;
    case '<':
        if(decrease_value_in_range(non_, 0, (key_count >= 15 ? 5 : 1), key_count==1)) {
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
