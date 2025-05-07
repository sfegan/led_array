#pragma once

#include "menu.hpp"

class RebootMenu: public FramedMenu {
public:
    RebootMenu(Menu* base_menu = nullptr);
    virtual ~RebootMenu() { }
    void redraw() override;
    bool controller_disconnected(int& return_code) override;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters,
        absolute_time_t& next_timer) override;
    bool process_timer(bool controller_is_connected, int& return_code,
        absolute_time_t& next_timer) override;
private:
    Menu* base_menu_ = nullptr;
    int dots_ = 0;
    int timer_calls_ = 0;
    bool first_redraw_ = true;
};

