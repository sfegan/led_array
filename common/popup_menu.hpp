#pragma once

#include "menu.hpp"

class PopupMenu: public FramedMenu {
public:
    PopupMenu(const std::string& message, unsigned timeout_sec = 0,
        bool press_to_quit = false, Menu* base_menu = nullptr, 
        const std::string& title = "Popup");
    virtual ~PopupMenu() { }
    void redraw() override;
    bool controller_disconnected(int& return_code) override;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters,
        absolute_time_t& next_timer) override;
    bool process_timer(bool controller_is_connected, int& return_code,
        absolute_time_t& next_timer) override;
private:
    Menu* base_menu_ = nullptr;
    std::string message_;
    unsigned timeout_sec_ = 0; 
    bool press_to_quit_ = false;

    unsigned timer_count_ = 0;
    unsigned timer_interval_ = 0;
    unsigned dot_count_ = 0;
    bool first_redraw_ = true;
};

