#pragma once

#include <string>
#include <vector>

#include <pico/stdlib.h>

#include "../common/color_led.hpp"
#include "../common/led_panel.hpp"
#include "../common/menu.hpp"
#include "../common/saved_state.hpp"

class BannerMenu: public SimpleItemValueMenu, public SavedStateSupplierConsumer {
public:
    enum class PlayMode : int { Repeat = 0, Alternate = 1, Stopped = 2 };

    BannerMenu(SerialPIO& pio, SavedStateManager* saved_state_manager = nullptr);
    virtual ~BannerMenu();

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
        MIP_ACTIVE_MSG,
        MIP_PLAY_MODE,
        MIP_TEXT,
        MIP_R,
        MIP_G,
        MIP_B,
        MIP_H,
        MIP_S,
        MIP_V,
        MIP_SCROLL_SPEED,
        MIP_CORNER,
        MIP_EXT_TRIGGER,
        MIP_WRITE_STATE,
        MIP_EXIT,
        MIP_NUM_ITEMS
    };

    static std::vector<MenuItem> make_menu_items();
    void set_values(bool draw = true);
    void set_active_msg_value(bool draw = true);
    void set_play_mode_value(bool draw = true);
    void set_text_value(bool draw = true);
    void set_ext_trigger_value(bool draw = true);
    void select_message(int index);
    void reset_scroll();
    void send_panel();
    void configure_trigger_pins();
    static void gpio_callback(uint gpio, uint32_t events);

    uint32_t active_color() const;
    const char* corner_name() const;

    SerialPIO& pio_;
    SavedStateManager* saved_state_manager_ = nullptr;
    LEDPanel panel_;
    RGBHSVMenuItems colors_[2];

    int active_message_ = 0; // 0 or 1
    PlayMode mode_ = PlayMode::Repeat;
    std::string messages_[2];
    bool external_trigger_enabled_ = true;
    int scroll_period_ms_ = 50;
    int scroll_x_ = 0;
    int static_dwell_ticks_ = 0;
    unsigned frame_ = 0;
    uint64_t last_trigger_us_ = 0;
    volatile int pending_message_select_ = -1;

    static BannerMenu* active_menu_;
};
