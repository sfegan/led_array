#include <string>
#include <vector>
#include <algorithm>

#include <cstring>
#include <cstdio>
#include <cctype>

#include <pico/time.h>
#include <pico/stdlib.h>
#include <pico/stdio.h>
#include <hardware/watchdog.h>

#include "build_date.hpp"
#include "menu.hpp"
#include "reboot_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

RebootMenu::RebootMenu(Menu* base_menu): 
    FramedMenu("Reboot",7,40,0), base_menu_(base_menu)
{ 
    cls_on_redraw_ = false;
}

void RebootMenu::redraw()
{
    if(base_menu_ and not first_redraw_) { base_menu_->redraw(); }
    first_redraw_ = false;
    FramedMenu::redraw();
    curpos(frame_r_+5, frame_c_+4);
    puts_raw_nonl("Hold ctrl-b to reboot : ");
    for(int i=0;i<dots_;++i)putchar_raw('X');
    for(int i=dots_;i<10;++i)putchar_raw('_');
}

bool RebootMenu::process_key_press(int key, int key_count, int& return_code, 
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    if(key == '\002') {
        ++dots_;
        curpos(frame_r_+5, frame_c_+28);
        for(int i=0;i<dots_;++i)putchar_raw('X');
        for(int i=dots_;i<10;++i)putchar_raw('_');
        if(dots_ >= 10) {
            watchdog_enable(1,false);
            while(1);
        }
        timer_calls_ = 0;
        return true;
    } else {
        curpos(frame_r_+5, frame_c_+ 4);
        puts_center_filled("  CANCELLED  ", frame_w_-6,'X');
        sleep_ms(1000);
        return_code = 0;
        return false;
    }
}

bool RebootMenu::controller_disconnected(int& return_code)
{
    return false;
}

bool RebootMenu::process_timer(bool controller_is_connected, int& return_code,
    absolute_time_t& next_timer)
{
    if(not controller_is_connected or timer_calls_>100)
    {
        curpos(frame_r_+5, frame_c_+ 4);
        puts_center_filled("  CANCELLED  ", frame_w_-6,'X');
        sleep_ms(1000);
        return_code = 0;
        return false;
    }
    ++timer_calls_;
    return true;
}

