#pragma once

#include <vector>

#include "../common/menu.hpp"
#include "../common/color_led.hpp"

class MainMenu: public SimpleItemValueMenu {
public:
    MainMenu();
    virtual ~MainMenu();
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters,
        absolute_time_t& next_timer) final;
    bool process_timer(bool controller_is_connected, int& return_code,
        absolute_time_t& next_timer) final;

private:
    enum MenuItemPositions {
        MIP_PIN,
        MIP_BAUDRATE,
        MIP_BLANK,
        MIP_MONO_COLOR,
        MIP_BI_COLOR,
        MIP_REBOOT,
        MIP_NUM_ITEMS // MUST BE LAST ITEM IN LIST
    };

    void set_pin_value(bool draw = true);
    void set_baudrate_value(bool draw = true);

    static std::vector<MenuItem> make_menu_items();

    SerialPIO pio_;
};
