#pragma once

#include <vector>
#include <list>

#include <pico/stdlib.h>

#include "../common/menu.hpp"
#include "../common/color_led.hpp"
#include "../common/saved_state.hpp"

class SpiderRunMenu: public SimpleItemValueMenu, public SavedStateSupplierConsumer {
public:
    SpiderRunMenu(SerialPIO& pio_, SavedStateManager* saved_state_manager = nullptr);
    virtual ~SpiderRunMenu() { }
    bool event_loop_starting(int& return_code) final;
    void event_loop_finishing(int& return_code) final;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters, absolute_time_t& next_timer) final;
    bool process_timer(bool controller_is_connected, int& return_code, absolute_time_t& next_timer) final;

    std::vector<int32_t> get_saved_state() override;
    bool set_saved_state(const std::vector<int32_t>& state) override;
    int32_t get_version() override;
    int32_t get_supplier_id() override;

private:
    enum MenuItemPositions {
        MIP_R,
        MIP_G,
        MIP_B,
        MIP_H,
        MIP_S,
        MIP_V,
        MIP_SPAWN_RATE,
        MIP_MAX_UPDATE,
        MIP_MIN_UPDATE,
        MIP_COLLISION,
        MIP_WRITE_STATE,
        MIP_EXIT,
        MIP_NUM_ITEMS // MUST BE LAST ITEM IN LIST
    };

    std::vector<MenuItem> make_menu_items();

    void set_spawn_value(bool draw = true);
    void set_max_update_value(bool draw = true);
    void set_min_update_value(bool draw = true);
    void set_collision_value(bool draw = true);

    void send_color_string();

    SerialPIO& pio_;
    SavedStateManager* saved_state_manager_ = nullptr;

    RGBHSVMenuItems c_;

    int spawn_rate_ = 20;
    int max_update_ = 10;
    int min_update_ = 10;
    bool collision_ = false;

    struct Spider {
        int x;
        int v;
        int creation;
    };

    int heartbeat_timer_count_ = 0;
    std::vector<uint32_t> color_codes_;
    std::vector<int> flash_value_;
    std::list<Spider> spiders_;

    std::minstd_rand rng_;
};
