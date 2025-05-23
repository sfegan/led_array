#pragma once

#include <vector>

#include "../common/menu.hpp"
#include "../common/color_led.hpp"
#include "../common/saved_state.hpp"

#include "mono_color_menu.hpp"
#include "bi_color_menu.hpp"

class MainMenu: public SimpleItemValueMenu,
                public SavedStateSupplierConsumer,
                public SavedStateManager {
public:
    MainMenu();
    virtual ~MainMenu();
    
    bool event_loop_starting(int& return_code) final;
    void event_loop_finishing(int& return_code) final;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters,
        absolute_time_t& next_timer) final;
    bool process_timer(bool controller_is_connected, int& return_code,
        absolute_time_t& next_timer) final;

    std::vector<int32_t> get_saved_state() override;
    bool set_saved_state(const std::vector<int32_t>& state) override;
    int32_t get_version() override;
    int32_t get_supplier_id() override;

    int32_t get_application_id() override;

private:
    enum MenuItemPositions {
        MIP_PIO,
        MIP_BLANK,
        MIP_MONO_COLOR,
        MIP_BI_COLOR,
        MIP_WRITE_STATE,
        MIP_REBOOT,
        MIP_NUM_ITEMS // MUST BE LAST ITEM IN LIST
    };

    bool process_menu_item(int key);

    static std::vector<MenuItem> make_menu_items();

    SerialPIOMenu pio_;
    MonoColorMenu mono_color_menu_;
    BiColorMenu bi_color_menu_;

    int32_t selected_menu_ = 0;
};
