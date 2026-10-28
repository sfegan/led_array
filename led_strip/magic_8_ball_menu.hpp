#pragma once

#include <random>
#include <string>
#include <vector>

#include "../common/color_led.hpp"
#include "../common/led_panel.hpp"
#include "../common/menu.hpp"
#include "../common/saved_state.hpp"

class Magic8BallMenu: public SimpleItemValueMenu, public SavedStateSupplierConsumer {
public:
    Magic8BallMenu(SerialPIO& pio, SavedStateManager* saved_state_manager = nullptr);
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
    enum MenuItemPositions { MIP_COLOR, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V,
        MIP_THINK_TIME, MIP_SCROLL_SPEED, MIP_CORNER,
        MIP_CURSOR_ROWS, MIP_FONT, MIP_PROMPT, MIP_TRIGGER_NOW, MIP_STATE, MIP_WRITE_STATE, MIP_EXIT, MIP_NUM_ITEMS };
    enum class State { Idle, Thinking, Scrolling };
    enum ColorSet { ShimmerA, ShimmerB, Positive, Uncertain, Negative, Cursor, ColorSetCount };

    static std::vector<MenuItem> make_menu_items();
    void set_values(bool draw = true);
    void send_panel();
    void start_thinking();
    void choose_response();
    void configure_trigger_pin();
    void handle_trigger();
    void set_color_set(bool draw = true);
    void set_font_spacing_value(bool draw = true);
    void set_state_value(bool draw = true);
    std::string state_value() const;
    int max_prompt_length() const;
    uint32_t color(ColorSet set) const;
    const char* color_set_name() const;
    const char* corner_name() const;
    static void gpio_callback(uint gpio, uint32_t events);

    SerialPIO& pio_;
    SavedStateManager* saved_state_manager_;
    LEDPanel panel_;
    RGBHSVMenuItems colors_[ColorSetCount];
    ColorSet color_set_ = ShimmerA;
    State state_ = State::Idle;
    int think_time_tenths_ = 30;
    int scroll_period_ms_ = 50;
    int cursor_rows_ = 8;
    std::string prompt_;
    int think_ticks_ = 0;
    int forced_response_ = 0; // 0 random, 1 positive, 2 negative
    int scroll_x_ = 0;
    int response_index_ = 0;
    unsigned frame_ = 0;
    uint64_t last_trigger_us_ = 0;
    std::string last_state_value_;
    std::minstd_rand rng_{0x8ba11};
    static Magic8BallMenu* active_menu_;
};
