#include <algorithm>
#include <string>

#include <cstring>
#include <cstdio>
#include <cctype>

#include <pico/time.h>
#include <pico/stdlib.h>
#include <pico/stdio.h>

#include "build_date.hpp"
#include "menu.hpp"
#include "input_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

InplaceInputMenu::InplaceInputMenu(RowAndColumnGetter& row_col_getter, unsigned max_value_size, 
        ValidInput valid_input, bool do_highlight, Menu* base_menu):
    InplaceInputMenu(row_col_getter.row(),row_col_getter.col(),
        max_value_size,valid_input,do_highlight,base_menu,&row_col_getter)
{
    // nothing to see here
}

InplaceInputMenu::InplaceInputMenu(int r, int c, unsigned max_value_size, ValidInput valid_input, 
        bool do_highlight, Menu* base_menu, RowAndColumnGetter* row_col_getter):
    Menu(base_menu ? base_menu->timer_interval_us() : 10000 /* 100Hz */), 
    base_menu_(base_menu), row_col_getter_(row_col_getter),
    r_(r), c_(c), max_value_size_(max_value_size), valid_input_(valid_input),
    do_highlight_(do_highlight), blink_interval_(std::max(1000000ULL / timer_interval_us(), 1ULL))
{
    // nothing to see here
}

InplaceInputMenu::~InplaceInputMenu()
{
    // nothing to see here
}

bool InplaceInputMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    switch(key) {
    case KEY_DELETE:
    case 8:   // Ctrl-h or backspace
    case 127: // Delete
        if(value_.size()) {
            value_.pop_back();
            draw_value();
        } else if(key_count == 1) {
            beep();
        }
        break;
    case 21:
        if(value_.size()) {
            value_.clear();
            draw_value();
        }
        break;
    case 3: // Ctrl-c
    case 4: // Ctrl-d
    case 27: // Esc
        return_code = 0;
        return false;
    case '\n':
    case '\r':
        return_code = 1;
        return false;
    default:
        if(value_.size() < max_value_size_ and is_valid(key)) {
            value_.push_back(key);
            draw_value();
        } else if(key_count == 1) {
            beep();
        }
        break;
    }
    return true;
}

bool InplaceInputMenu::is_valid(int key)
{
    if(key>127) {
        return false;
    }
    switch(valid_input_) {
    case VI_STRING:
        return isprint(key);
    case VI_FLOAT:        
        if(value_.size()==0)return key=='-' or key=='.' or isdigit(key);
        else if(value_.size()==1) {
            if(value_[0] == '-')return key=='.' or isdigit(key);
            else if(value_[0]=='0')return key=='.';
            else if(value_[0]=='.')return isdigit(key);
            else return isdigit(key) or key=='.';
        } else if(value_.size()==2) {
            if(value_[0] == '-') {
                if(value_[1]=='0')return key=='.';
                else if(value_[1]=='.')return isdigit(key);
                else return isdigit(key) or key=='.';
            } else {
                if(key == '.') {
                    return value_.find('.') == std::string::npos;
                } else {
                    return isdigit(key);
                }
            } 
        } else {
            if(key == '.') {
                return value_.find('.') == std::string::npos;
            } else {
                return isdigit(key);
            }
        }
        return false;
    case VI_POSITIVE_FLOAT:        
        if(value_.size()==0)return key=='.' or isdigit(key);
        else if(value_.size()==1) {
            if(value_[0]=='0')return key=='.';
            else if(value_[0]=='.')return isdigit(key);
            else return isdigit(key) or key=='.';
        } else {
            if(key == '.') {
                return value_.find('.') == std::string::npos;
            } else {
                return isdigit(key);
            }
        }
        return false;
    case VI_INTEGER:
        if(value_.size()==0)return key=='-' or isdigit(key);
        else if(value_.size()==1)return isdigit(key) and (value_[0]!='0' and key!='0');
        else return isdigit(key);
    case VI_NATURAL:
        if(value_.size()==0)return isdigit(key);
        else return isdigit(key) and value_[0]!='0';
    }
    return false;
}

bool InplaceInputMenu::process_timer(bool controller_is_connected, int& return_code,
    absolute_time_t& next_timer)
{
    blink_count_ += 1;
    if(blink_count_ >= blink_interval_) {
        blink_count_ = 0;
        if(controller_is_connected) {
            blink_on_ = !blink_on_;
            if(value_.size() < max_value_size_) {
                curpos(r_+1, c_+1+value_.size());
                if(do_highlight_) {
                    save_cursor();
                    highlight();
                }
                putchar_raw(blink_on_ ? ' ' : '_');
                if(do_highlight_) {
                    restore_cursor();
                }
            }
        }
    }
    if(base_menu_) {
        controller_is_connected &= base_menu_->process_timer(
            controller_is_connected, return_code, next_timer);
    }
    return controller_is_connected;
}

void InplaceInputMenu::redraw()
{
    if(base_menu_ and not first_redraw_) { base_menu_->redraw(); }
    first_redraw_ = false;
    if(row_col_getter_) { 
        r_ = row_col_getter_->row();
        c_ = row_col_getter_->col(); 
    }
    draw_value();
}

void InplaceInputMenu::draw_value()
{
    curpos(r_+1, c_+1);
    if(do_highlight_) {
        save_cursor();
        highlight();
    }
    for(unsigned i=0;i<value_.size();++i)putchar_raw(value_[i]);
    if(value_.size() < max_value_size_) {
        putchar_raw(blink_on_ ? ' ' : '_');
    }
    for(unsigned i=value_.size()+1;i<max_value_size_;++i)putchar_raw('_');
    if(do_highlight_) {
        restore_cursor();
    }
}

void InplaceInputMenu::cancelled()
{
    curpos(r_+1, c_+1);
    if(do_highlight_) {
        save_cursor();
        highlight();
    }
    for(unsigned i=0;i<max_value_size_;++i)putchar_raw('X');
    if(do_highlight_) {
        restore_cursor();
    }
    sleep_ms(750);
}

bool InplaceInputMenu::input_value_in_range(int& value, int value_min, int value_max,
    SimpleItemValueMenu* base_menu, int iitem, unsigned max_value_size)
{
    ValidInput vi = value_min<0 ? VI_INTEGER : VI_NATURAL;
    if(max_value_size == 0) {
        unsigned value_max_size = 0;
        for(int v = std::abs(value_max); v>0;  value_max_size+=1, v/=10);
        if(value_max<0) {
            value_max_size += 1;
        }
        unsigned value_min_size = 0;
        for(int v = std::abs(value_min); v>0;  value_min_size+=1, v/=10);
        if(value_min<0) {
            value_min_size += 1;
        }
        max_value_size = std::max(value_min_size, value_max_size);
    }
    SimpleItemValueRowAndColumnGetter rc_getter(base_menu, iitem);
    InplaceInputMenu input(rc_getter, max_value_size, vi, true, base_menu);
    if(input.event_loop()==1 and input.get_value().size()!=0) {
        int val = std::stoi(input.get_value());
        if(val>=value_min and val<=value_max) {
            value = val;
            return true;
        } else {
            beep();
            input.cancelled();
        }
    } else {
        input.cancelled();
    }
    return false;
}

InputMenu::InputMenu(unsigned max_value_size, ValidInput valid_input,
        const std::string title, const std::string prompt, Menu* base_menu):
    FramedMenu(title,7,std::max({40U,title.size()+6U,max_value_size+prompt.size()+7U})), 
    iim_(*this, max_value_size, valid_input, false, nullptr),
    base_menu_(base_menu), prompt_(prompt)
{
    timer_interval_us_ = iim_.timer_interval_us();
    cls_on_redraw_ = false;
}

InputMenu::InputMenu(unsigned max_value_size, const std::string title,
        const std::string prompt, ValidInput valid_input, Menu* base_menu):
    InputMenu(max_value_size, valid_input, title, prompt, base_menu)
{
    // nothing to see here
}

InputMenu::~InputMenu()
{
    // nothing to see here
}

bool InputMenu::event_loop_starting(int& return_code)
{
    return iim_.event_loop_starting(return_code);
}

void InputMenu::event_loop_finishing(int& return_code)
{
    iim_.event_loop_finishing(return_code);
}

bool InputMenu::controller_connected(int& return_code)
{
    return iim_.controller_connected(return_code);
}

bool InputMenu::controller_disconnected(int& return_code)
{
    return iim_.controller_disconnected(return_code);
}

bool InputMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters,
    absolute_time_t& next_timer)
{
    return iim_.process_key_press(key, key_count, return_code, escape_sequence_parameters,
        next_timer);
}

bool InputMenu::process_timer(bool controller_is_connected, int& return_code,
    absolute_time_t& next_timer)
{
    return iim_.process_timer(controller_is_connected, return_code, next_timer);
}

void InputMenu::redraw()
{
    if(base_menu_ and not first_redraw_) { base_menu_->redraw(); }
    first_redraw_ = false;
    FramedMenu::redraw();
    curpos(frame_r_+5, frame_c_+4);
    puts_raw_nonl(prompt_);
    iim_.redraw();
}

const std::string& InputMenu::get_value() const
{
    return iim_.get_value();
}

void InputMenu::cancelled()
{
    curpos(frame_r_+5, frame_c_+ 4);
    puts_center_filled("  CANCELLED  ", frame_w_-6,'X');
    sleep_ms(750);
}

int InputMenu::row()
{
    return frame_r_+4;
}

int InputMenu::col()
{
    return frame_c_+prompt_.size()+4;
}
