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

int Menu::screen_w_ = Menu::default_screen_width();
int Menu::screen_h_ = Menu::default_screen_height();

RowAndColumnGetter::~RowAndColumnGetter()
{
    // nothing to see here
}

Menu::~Menu() 
{ 
    // nothing to see here
}

bool Menu::event_loop_starting(int& return_code)
{
    return true;
}

void Menu::event_loop_finishing(int& return_code)
{
    // nothing to see here
}

bool Menu::controller_connected(int& return_code)
{
    return true;
}

bool Menu::controller_disconnected(int& return_code)
{
    return true;
}

int Menu::puts_raw_nonl(const char* s) 
{
    for (size_t i = 0; s[i]; ++i) {
        if (putchar_raw(s[i]) == EOF) return EOF;
    }
    return 0;
}

int Menu::puts_raw_nonl(const char* s, size_t maxchars, bool fill) 
{
    for (size_t i = 0; s[i] && maxchars; ++i, --maxchars) {
        if (putchar_raw(s[i]) == EOF) return EOF;
    }
    if(fill && maxchars) {
        while(maxchars--) {
            if (putchar_raw(' ') == EOF) return EOF;
        }
    }
    return 0;
}

int Menu::puts_raw_nonl(const std::string& s) {
    for (size_t i=0; i<s.size(); ++i) {
        if (putchar_raw(s[i]) == EOF) return EOF;
    }
    return 0;
}

int Menu::puts_raw_nonl(const std::string& s, size_t maxchars, bool fill) 
{
    size_t schars = std::min(maxchars, s.size());
    for (size_t i=0; i<schars; ++i, --maxchars) {
        if (putchar_raw(s[i]) == EOF) return EOF;
    }
    if(fill && maxchars) {
        while(maxchars--) {
            if (putchar_raw(' ') == EOF) return EOF;
        }
    }
    return 0;
}

int Menu::puts_formatted(const std::string& s, const std::string& format, 
    size_t maxchars, bool fill)
{
    size_t schars = std::min(maxchars, s.size());
    if(!format.empty()) {
        if(puts_raw_nonl("\0337") == EOF or puts_raw_nonl(format) == EOF)
            return EOF;
    }
    for (size_t i=0; i<schars; ++i, --maxchars) {
        if (putchar_raw(s[i]) == EOF) return EOF;
    }
    if(fill && maxchars) {
        while(maxchars--) {
            if (putchar_raw(' ') == EOF) return EOF;
        }
    }
    if(!format.empty()) {
        if (puts_raw_nonl("\0338") == EOF)return EOF;
    }
    return 0;
}

int Menu::puts_center_filled(const std::string& s, size_t maxchars, char fill_char)
{
    size_t schars = std::min(maxchars, s.size());
    size_t fchars = (maxchars-schars)/2;
    for (size_t i=0; i<fchars; ++i, --maxchars) {
        if (putchar_raw(fill_char) == EOF) return EOF;
    }
    for (size_t i=0; i<schars; ++i, --maxchars) {
        if (putchar_raw(s[i]) == EOF) return EOF;
    }
    while(maxchars--) {
        if (putchar_raw(fill_char) == EOF) return EOF;
    }
    return 0;
}


void Menu::cls() 
{ 
    puts_raw_nonl("\033[2J"); 
}

void Menu::show_cursor() 
{ 
    puts_raw_nonl("\033[?25h"); //\0337p"); 
}

void Menu::hide_cursor() 
{ 
    puts_raw_nonl("\033[?25l"); //\0336p"); 
}

void Menu::curpos(int r, int c) 
{ 
    char buffer[80]; 
    sprintf(buffer,"\033[%d;%dH",r,c); 
    puts_raw_nonl(buffer);
}

void Menu::save_cursor()
{
    puts_raw_nonl("\0337");
}

void Menu::restore_cursor()
{
    puts_raw_nonl("\0338");
}

void Menu::highlight()
{
    puts_raw_nonl(ANSI_INVERT);
}

void Menu::reset_colors()
{
    puts_raw_nonl("\033[m");
}

void Menu::send_request_screen_size() 
{
    puts_raw_nonl("\0337\033[999;999H\033[6n\0338");
}

void Menu::beep()
{
    putchar_raw(7);
}

void Menu::draw_box(int fh, int fw, int fr, int fc) {
    curpos(fr+1,fc+1);
    putchar_raw('+');
    for(int ic=2;ic<fw;++ic)putchar_raw('-');
    putchar_raw('+');
    for(int ir=2;ir<fh;++ir) {
        curpos(fr+ir,fc+1);
        putchar_raw('|');
        for(int ic=2;ic<fw;++ic)putchar_raw(' ');
        putchar_raw('|');
    }
    curpos(fr+fh,fc+1);
    putchar_raw('+');
    for(int ic=2;ic<fw;++ic)putchar_raw('-');
    putchar_raw('+');
}

bool Menu::draw_title(const std::string& title, int fh, int fw, int fr, int fc,
     const std::string& title_style) 
{
    char buffer[80];
    if(fh<5 || fw<5)return false;
    int tw = std::min(int(title.size()), fw-4);
    int tc = fc + (fw-tw)/2;
    sprintf(buffer,"\033[%d;%dH",fr+3,tc+1);
    puts_raw_nonl(buffer);
    if(!title_style.empty())puts_raw_nonl(title_style);
    puts_raw_nonl(title, tw);
    if(!title_style.empty())puts_raw_nonl("\033[0m");
    return tw == int(title.size());
}

bool Menu::draw_heart(bool on, int fh, int fw, int fr, int fc)
{
    if(fh<2 || fw<6)return false;
    curpos(fr+fh+1, fc+fw+1-4);
    if(on)puts_raw_nonl("<3");
    else puts_raw_nonl("--");
    return true;
}

bool Menu::process_rc_keys(int& ar, int& ac, int key, int key_count)
{
    switch(key) {
    case KEY_UP:
        decrease_value_in_range(ar, 0, 1, key_count==1);
        return true;
    case KEY_DOWN:
        increase_value_in_range(ar, 15, 1, key_count==1);
        return true;
    case KEY_LEFT:
        Menu::decrease_value_in_range(ac, 0, 1, key_count==1);
        return true;
    case KEY_RIGHT:
        increase_value_in_range(ac, 15, 1, key_count==1);
        return true;
    case KEY_PAGE_UP:
        ar = 0;
        return true;
    case KEY_PAGE_DOWN:
        ar = 15;
        return true;
    case KEY_HOME:
        ac = 0;
        return true;
    case KEY_END:
        ac = 15;
        return true;
    }
    return false;
}

void Menu::rc_to_value_string(std::string& value, int ar, int ac)
{
    value = std::string(1, char('A' + ar)) + std::to_string(ac); 
}

FramedMenu::FramedMenu(const std::string& title, int frame_h, int frame_w, int frame_pos, uint64_t timer_interval_us):
    Menu(timer_interval_us), title_(title), req_h_(frame_h), req_w_(frame_w), req_pos_(frame_pos)
{
    // nothing to see here
}

FramedMenu::~FramedMenu()
{
    // nothing to see here
}

void FramedMenu::redraw()
{
    setup_frame();
    hide_cursor();
    if(cls_on_redraw_)cls();
    draw_box(frame_h_, frame_w_, frame_r_, frame_c_);
    if(!title_.empty())draw_title(title_, frame_h_, frame_w_, frame_r_, frame_c_, {});
    if(heartbeat_) {
        draw_heart(true, frame_h_, frame_w_, frame_r_, frame_c_);
    }
}

void FramedMenu::set_heartbeat(bool on)
{
    if(heartbeat_ != on) {
        heartbeat_ = on;
        draw_heart(heartbeat_, frame_h_, frame_w_, frame_r_, frame_c_);
    }
}

void FramedMenu::setup_frame()
{
    frame_h_ = (req_h_>0) ? std::min(screen_h_, req_h_) : screen_h_;
    frame_w_ = (req_w_>0) ? std::min(screen_w_, req_w_) : screen_w_;
    switch(req_pos_) {
        case 0: case 3: case 7: default:
            frame_r_ = (screen_h_-frame_h_)/2; break;
        case 1: case 2: case 8: 
            frame_r_ = 0; break;
        case 4: case 5: case 6: 
            frame_r_ = screen_h_-frame_h_; break;
    }
    switch(req_pos_) {
        case 0: case 1: case 5: default:
            frame_c_ = (screen_w_-frame_w_)/2; break;
        case 2: case 3: case 4: 
            frame_c_ = screen_w_-frame_w_; break;
        case 6: case 7: case 8: 
            frame_c_ = 0; break;
    }
}

SimpleItemValueMenu::MenuItem::
MenuItem(const std::string& item_, int max_value_size_, const std::string& value_):
    item(item_), max_value_size(max_value_size_), value(value_)
{ 
    // nothing to see here
}

SimpleItemValueMenu::SimpleItemValueMenu(const std::vector<MenuItem>& menu_items, 
        const std::string& title, int frame_h, int frame_w, int frame_pos):
    FramedMenu(title, frame_h, frame_w, frame_pos), menu_items_(menu_items)
{
    // nothing to see here
}

SimpleItemValueMenu::~SimpleItemValueMenu()
{
    // nothing to see here
}

void SimpleItemValueMenu::setup_menu()
{
    item_r_ = frame_r_ + (title_.empty() ? 2 : 4);
    for(const auto& i : menu_items_) {
        item_w_ = std::max(item_w_, int(i.item.size()));
        val_w_ = std::max(val_w_, int(i.max_value_size));        
    }
    item_count_ = menu_items_.size();
    if(2*item_count_+item_r_+1 < frame_h_) {
        item_h_ = 2*item_count_-1;
        item_dr_ = 2;
    } else {
        item_h_ = std::min(item_count_, frame_h_-item_r_-2);
        item_dr_ = 1;
        item_count_ = item_h_;
    }
    item_w_ = std::min(item_w_, frame_w_ - val_w_ - 6);
    item_r_ += (frame_h_ - item_h_ - item_r_ - 2)/2;
    item_c_ = frame_c_ + std::min(5, frame_w_-(item_w_+val_w_+6)/2);
    val_c_ = frame_c_ + frame_w_ - val_w_ - (item_c_ - frame_c_);
}

int SimpleItemValueMenu::get_item_value_row(int iitem)
{
    return item_r_+iitem*item_dr_;
}

int SimpleItemValueMenu::get_item_value_col(int iitem)
{
    return val_c_;
}

void SimpleItemValueMenu::redraw()
{
    FramedMenu::redraw();
    setup_menu();
    for(int iitem=0;iitem<item_count_;++iitem) {
        draw_item(iitem);
    }
}

void SimpleItemValueMenu::draw_item(unsigned iitem)
{
    curpos(item_r_+iitem*item_dr_+1, item_c_+1);
    if(menu_items_[iitem].max_value_size > 0) {
        puts_raw_nonl(menu_items_[iitem].item, item_w_);
        putchar_raw(' ');
        for(int ic = item_c_+menu_items_[iitem].item.size()+2; ic<val_c_; ic ++)
            putchar_raw('.');
        putchar_raw(' ');
        draw_item_value(iitem);
    } else {
        puts_raw_nonl(menu_items_[iitem].item, item_w_+val_w_+2);
    }
}

void SimpleItemValueMenu::draw_item_value(unsigned iitem)
{
    if(iitem<menu_items_.size()) {
        curpos(item_r_+iitem*item_dr_+1, val_c_+1);
        if(!menu_items_[iitem].value_style.empty()) {
            puts_formatted(menu_items_[iitem].value, menu_items_[iitem].value_style, 
                menu_items_[iitem].max_value_size, true);
        } else {
            puts_raw_nonl(menu_items_[iitem].value, menu_items_[iitem].max_value_size, true);
        }
    }
}

SimpleItemValueRowAndColumnGetter::~SimpleItemValueRowAndColumnGetter()
{
    // nothing to see here
}

int SimpleItemValueRowAndColumnGetter::row()
{
    return menu_->get_item_value_row(iitem_);
}

int SimpleItemValueRowAndColumnGetter::col()
{
    return menu_->get_item_value_col(iitem_);
}
