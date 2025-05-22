#pragma once

#include <vector>
#include <random>

#include <pico/stdlib.h>

#include "../common/menu.hpp"
#include "../common/color_led.hpp"
#include "../common/saved_state.hpp"

class BiColorMenu: public SimpleItemValueMenu, public SavedStateSupplierConsumer {
public:
BiColorMenu(SerialPIO& pio_);
    virtual ~BiColorMenu() { }
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
        MIP_SWITCH,
        MIP_R,
        MIP_G,
        MIP_B,
        MIP_H,
        MIP_S,
        MIP_V,
        MIP_PERIOD,
        MIP_HOLD,
        MIP_BALANCE,
        MIP_SPEED,
        MIP_FLASH_PROB,
        MIP_EXIT,
        MIP_NUM_ITEMS // MUST BE LAST ITEM IN LIST
    };

    std::vector<MenuItem> make_menu_items();

    void set_cset_value(bool draw = true);
    void set_period_value(bool draw = true);
    void set_hold_value(bool draw = true);
    void set_balance_value(bool draw = true);
    void set_speed_value(bool draw = true);
    void set_flash_prob_value(bool draw = true);
    
    void generate_random_flashes();
    uint32_t color_code(int iled, bool debug = false);
    void send_color_string(bool flash = false);

    SerialPIO& pio_;
    RGBHSVMenuItems c0_;
    RGBHSVMenuItems c1_;

    int cset_ = 0;
    int period_ = 20;
    int hold_ = 0;
    int balance_ = 0;
    int speed_ = 0;
    int phase_ = 0;
    int flash_prob_ = 0;

    int heartbeat_timer_count_ = 0;
    std::vector<uint32_t> color_codes_;
    std::vector<int> flash_value_;

    // All calculations in integer math, using 0..65535 for fractions
    static constexpr int FRAC_BITS = 16;
    static constexpr int FRAC_ONE = 1 << FRAC_BITS;

    void update_calculations();

    int p_len_;
    int trans_len_;
    int up_start_;
    int up_end_;
    int c1_hold_end_;
    int down_end_;
    uint32_t non_flash_prob_ = 0;

    std::minstd_rand rng_;
};
