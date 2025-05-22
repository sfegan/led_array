#include <string>
#include <vector>
#include <algorithm>

#include <cstring>
#include <cstdio>
#include <cctype>

#include <pico/time.h>
#include <pico/stdlib.h>
#include <pico/stdio.h>

#include "build_date.hpp"
#include "menu.hpp"
#include "popup_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

PopupMenu::PopupMenu(const std::string& message, unsigned timeout_sec,
        bool press_to_quit, Menu* base_menu, const std::string& title): 
    FramedMenu(title,7,message.size()+timeout_sec+4,0), base_menu_(base_menu),
    message_(message), timeout_sec_(timeout_sec), 
    press_to_quit_(timeout_sec==0 or press_to_quit)
{ 
    cls_on_redraw_ = false;
}

void PopupMenu::redraw()
{
    if(base_menu_ and not first_redraw_) { base_menu_->redraw(); }
    first_redraw_ = false;
    FramedMenu::redraw();
    curpos(frame_r_+5, frame_c_+4);
    puts_raw_nonl(message_);
    for(int i=0;i<timer_calls_;++i)putchar_raw('.');
}

bool PopupMenu::process_key_press(int key, int key_count, int& return_code, 
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    if(press_to_quit_) {
        return_code = key;
        return false;
    }
    return true;
}

bool PopupMenu::controller_disconnected(int& return_code)
{
    return_code = 0;
    return false;
}

bool PopupMenu::process_timer(bool controller_is_connected, int& return_code,
    absolute_time_t& next_timer)
{
    if(timeout_sec_ > 0) {
        if(timer_calls_ >= timeout_sec_) {
            return_code = 0;
            return false;
        }
        curpos(frame_r_+5, frame_c_+2+message_.size()+timer_calls_);
        putchar_raw('.');
        ++timer_calls_;
    }
    return true;
}

