#include "banner_menu.hpp"

#include <algorithm>

#include <hardware/gpio.h>

#include "../common/input_menu.hpp"
#include "../common/popup_menu.hpp"

BannerMenu* BannerMenu::active_menu_ = nullptr;

BannerMenu::BannerMenu(SerialPIO& pio, SavedStateManager* saved_state_manager):
    SimpleItemValueMenu(make_menu_items(), "Scrolling banner"), pio_(pio),
    saved_state_manager_(saved_state_manager), panel_(32, 8),
    colors_{
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V)
    }
{
    timer_interval_us_ = 10000; // 100 Hz
    messages_[0] = "HELLO";
    messages_[1] = "WORLD";
    colors_[0].set_rgb(0, 96, 0, false);
    colors_[1].set_rgb(96, 48, 0, false);
    panel_.set_font_spacing(LEDPanel::FontSpacing::Proportional);
    set_values(false);
}

BannerMenu::~BannerMenu()
{
    // nothing to see here
}

std::vector<SimpleItemValueMenu::MenuItem> BannerMenu::make_menu_items()
{
    std::vector<MenuItem> items(MIP_NUM_ITEMS);
    items[MIP_ACTIVE_MSG]   = {"Space   : Active message", 10, "MESSAGE 1"};
    items[MIP_PLAY_MODE]    = {"a       : Play mode (REPEAT/ALTERNATE/STOPPED)", 9, "REPEAT"};
    items[MIP_FONT]         = {"f       : Font spacing", 12, "PROPORTIONAL"};
    items[MIP_TEXT]         = {"t       : Edit active message text", 24, ""};
    RGBHSVMenuItems::make_menu_items(items, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V);
    items[MIP_SCROLL_SPEED] = {"x       : Pixel scroll period [ms]", 3, "50"};
    items[MIP_CORNER]       = {"o/O     : Cycle LED-0 panel corner (serpentine)", 12, "TOP LEFT"};
    items[MIP_EXT_TRIGGER]  = {"e       : External switch trigger", 8, "<ON>"};
    items[MIP_WRITE_STATE]  = {"Ctrl-w  : Write state to flash", 0, ""};
    items[MIP_EXIT]         = {"q       : Exit menu", 0, ""};
    return items;
}

const char* BannerMenu::corner_name() const
{
    switch(panel_.corner()) {
    case LEDPanel::Corner::TopLeft: return "TOP LEFT";
    case LEDPanel::Corner::TopRight: return "TOP RIGHT";
    case LEDPanel::Corner::BottomLeft: return "BOTTOM LEFT";
    case LEDPanel::Corner::BottomRight: return "BOTTOM RIGHT";
    }
    return "TOP LEFT";
}

uint32_t BannerMenu::active_color() const
{
    return rgb_to_grbz(colors_[active_message_].r(),
                       colors_[active_message_].g(),
                       colors_[active_message_].b());
}

void BannerMenu::set_active_msg_value(bool draw)
{
    menu_items_[MIP_ACTIVE_MSG].value = active_message_ == 0 ? "MESSAGE 1" : "MESSAGE 2";
    if(draw) draw_item_value(MIP_ACTIVE_MSG);
}

void BannerMenu::set_play_mode_value(bool draw)
{
    const char* mode_str = "REPEAT";
    if(mode_ == PlayMode::Alternate) mode_str = "ALTERNATE";
    else if(mode_ == PlayMode::Stopped) mode_str = "STOPPED";
    menu_items_[MIP_PLAY_MODE].value = mode_str;
    if(draw) draw_item_value(MIP_PLAY_MODE);
}

void BannerMenu::set_font_spacing_value(bool draw)
{
    menu_items_[MIP_FONT].value = (panel_.font_spacing() == LEDPanel::FontSpacing::Proportional) ?
        "PROPORTIONAL" : "FIXED";
    if(draw) draw_item_value(MIP_FONT);
}

void BannerMenu::set_text_value(bool draw)
{
    const std::string& msg = messages_[active_message_];
    if(msg.size() > 24) {
        menu_items_[MIP_TEXT].value = msg.substr(0, 21) + "...";
    } else {
        menu_items_[MIP_TEXT].value = msg;
    }
    if(draw) draw_item_value(MIP_TEXT);
}

void BannerMenu::set_ext_trigger_value(bool draw)
{
    menu_items_[MIP_EXT_TRIGGER].set_onoff(external_trigger_enabled_);
    if(draw) draw_item_value(MIP_EXT_TRIGGER);
}

void BannerMenu::set_values(bool draw)
{
    set_active_msg_value(draw);
    set_play_mode_value(draw);
    set_font_spacing_value(draw);
    set_text_value(draw);
    colors_[active_message_].redraw(draw);
    menu_items_[MIP_SCROLL_SPEED].value = std::to_string(scroll_period_ms_);
    menu_items_[MIP_CORNER].value = corner_name();
    set_ext_trigger_value(draw);
    if(draw) {
        draw_item_value(MIP_SCROLL_SPEED);
        draw_item_value(MIP_CORNER);
    }
}

void BannerMenu::select_message(int index)
{
    if(index < 0 || index > 1) return;
    active_message_ = index;
    static_dwell_ticks_ = 0;
    set_values(true);
    reset_scroll();
    send_panel();
}

void BannerMenu::reset_scroll()
{
    static_dwell_ticks_ = 0;
    if(mode_ == PlayMode::Stopped) {
        scroll_x_ = 0;
        return;
    }
    const int tw = panel_.text_width(messages_[active_message_]);
    if(tw <= panel_.width()) {
        scroll_x_ = std::max(0, (panel_.width() - tw) / 2);
    } else {
        scroll_x_ = panel_.width();
    }
}

void BannerMenu::send_panel()
{
    panel_.clear();
    const std::string& text = messages_[active_message_];
    if(!text.empty()) {
        if(mode_ == PlayMode::Stopped) {
            panel_.draw_text(text, 0, active_color());
        } else {
            const int tw = panel_.text_width(text);
            if(tw <= panel_.width()) {
                const int x = std::max(0, (panel_.width() - tw) / 2);
                panel_.draw_text(text, x, active_color());
            } else {
                panel_.draw_text(text, scroll_x_, active_color());
            }
        }
    }

    std::vector<uint32_t> output(std::max(0, pio_.nled()), 0);
    const int active_start = pio_.back() ? pio_.nled() - pio_.non() : 0;
    const int copied = std::min({int(panel_.pixels().size()), pio_.non(), pio_.nled()});
    for(int i = 0; i < copied; ++i) output[active_start + i] = panel_.pixels()[i];
    pio_.put_pixel_vector(output);
    pio_.flush();
}

void BannerMenu::configure_trigger_pins()
{
    const auto configure = [](int pin) {
        if(pin < 0) return;
        gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, false);
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &BannerMenu::gpio_callback);
    };
    configure(pio_.trigger_line_1());
    if(pio_.trigger_line_2() != pio_.trigger_line_1()) {
        configure(pio_.trigger_line_2());
    }
}

void BannerMenu::gpio_callback(uint gpio, uint32_t events)
{
    if(!active_menu_ || !active_menu_->external_trigger_enabled_) return;

    if(events & GPIO_IRQ_EDGE_FALL) {
        const uint64_t now = time_us_64();
        if(now - active_menu_->last_trigger_us_ >= 100000) { // 100ms switch debounce
            active_menu_->last_trigger_us_ = now;
            if(gpio == unsigned(active_menu_->pio_.trigger_line_1())) {
                active_menu_->pending_message_select_ = 0;
            } else if(active_menu_->pio_.trigger_line_2() >= 0 &&
                      gpio == unsigned(active_menu_->pio_.trigger_line_2())) {
                active_menu_->pending_message_select_ = 1;
            }
        }
    }
}

bool BannerMenu::event_loop_starting(int&)
{
    if(pio_.non() >= 8 && pio_.non() % 8 == 0) {
        panel_.resize(pio_.non() / 8, 8);
    }
    active_menu_ = this;
    pending_message_select_ = -1;
    configure_trigger_pins();
    pio_.activate_program();
    reset_scroll();
    set_values(false);
    send_panel();
    return true;
}

void BannerMenu::event_loop_finishing(int&)
{
    gpio_set_irq_enabled(pio_.trigger_line_1(), GPIO_IRQ_EDGE_FALL, false);
    if(pio_.trigger_line_2() >= 0 && pio_.trigger_line_2() != pio_.trigger_line_1()) {
        gpio_set_irq_enabled(pio_.trigger_line_2(), GPIO_IRQ_EDGE_FALL, false);
    }
    active_menu_ = nullptr;
    pio_.put_pixel(0, pio_.nled());
    pio_.flush();
    pio_.deactivate_program();
}

bool BannerMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>&, absolute_time_t& next_timer)
{
    bool color_changed = false;
    if(colors_[active_message_].process_key_press(key, key_count, color_changed)) {
        if(color_changed) send_panel();
        return true;
    }

    switch(key) {
    case ' ':
        select_message(1 - active_message_);
        break;

    case 'a':
    case 'A':
        mode_ = static_cast<PlayMode>((static_cast<int>(mode_) + 1) % 3);
        set_play_mode_value(true);
        reset_scroll();
        send_panel();
        break;

    case 'f':
    case 'F':
        panel_.set_font_spacing(panel_.font_spacing() == LEDPanel::FontSpacing::Proportional ?
            LEDPanel::FontSpacing::Fixed : LEDPanel::FontSpacing::Proportional);
        set_font_spacing_value(true);
        reset_scroll();
        send_panel();
        break;

    case 't':
    case 'T': {
        InputMenu input(80, "Edit Message Text", "Text: ", VI_STRING, this);
        input.set_value(messages_[active_message_]);
        if(input.event_loop() == 1) {
            messages_[active_message_] = input.get_value();
            set_text_value(true);
            reset_scroll();
            send_panel();
        }
        // Discard accumulated timer events while the modal InputMenu was open
        next_timer = delayed_by_us(get_absolute_time(), timer_interval_us_);
        redraw();
        break;
    }

    case 'x':
        InplaceInputMenu::input_value_in_range(scroll_period_ms_, 10, 255, this, MIP_SCROLL_SPEED, 3);
        menu_items_[MIP_SCROLL_SPEED].value = std::to_string(scroll_period_ms_);
        draw_item_value(MIP_SCROLL_SPEED);
        break;

    case 'o':
        panel_.set_corner(LEDPanel::Corner((int(panel_.corner()) + 1) % 4));
        menu_items_[MIP_CORNER].value = corner_name();
        draw_item_value(MIP_CORNER);
        send_panel();
        break;

    case 'O':
        panel_.set_corner(LEDPanel::Corner((int(panel_.corner()) + 3) % 4));
        menu_items_[MIP_CORNER].value = corner_name();
        draw_item_value(MIP_CORNER);
        send_panel();
        break;

    case 'e':
    case 'E':
        external_trigger_enabled_ = !external_trigger_enabled_;
        set_ext_trigger_value(true);
        break;

    case 23: // Ctrl-w
        if(saved_state_manager_) {
            saved_state_manager_->save_state();
            PopupMenu pm("State written to flash", 2, true, this, "Information");
            pm.event_loop();
            // Discard accumulated timer events while PopupMenu was open
            next_timer = delayed_by_us(get_absolute_time(), timer_interval_us_);
            redraw();
        }
        break;

    case 'q':
    case 'Q':
        return_code = 0;
        return false;

    default:
        if(key_count == 1) beep();
        break;
    }

    return true;
}

bool BannerMenu::process_timer(bool controller_is_connected, int&, absolute_time_t&)
{
    ++frame_;

    if(pending_message_select_ >= 0) {
        int target = pending_message_select_;
        pending_message_select_ = -1;
        select_message(target);
    }

    if(mode_ == PlayMode::Stopped) {
        // When stopped, hold text statically at index 0 without scrolling or alternating.
    } else {
        const std::string& text = messages_[active_message_];
        const int tw = panel_.text_width(text);

        if(tw > panel_.width()) {
            if(frame_ % std::max(1, scroll_period_ms_ / 10) == 0) {
                --scroll_x_;
                if(scroll_x_ < -tw) {
                    if(mode_ == PlayMode::Alternate) {
                        select_message(1 - active_message_);
                    } else {
                        scroll_x_ = panel_.width();
                        send_panel();
                    }
                } else {
                    send_panel();
                }
            }
        } else {
            if(mode_ == PlayMode::Alternate) {
                if(++static_dwell_ticks_ >= 300) { // 3 seconds at 100 Hz
                    static_dwell_ticks_ = 0;
                    select_message(1 - active_message_);
                }
            }
        }
    }

    if(controller_is_connected && frame_ % 100 == 0) {
        set_heartbeat(!heartbeat_);
    }

    return true;
}

std::vector<int32_t> BannerMenu::get_saved_state()
{
    std::vector<int32_t> state = {
        active_message_,
        external_trigger_enabled_ ? 1 : 0,
        scroll_period_ms_,
        static_cast<int32_t>(panel_.corner()),
        colors_[0].r(), colors_[0].g(), colors_[0].b(),
        colors_[1].r(), colors_[1].g(), colors_[1].b(),
        static_cast<int32_t>(mode_),
        static_cast<int32_t>(panel_.font_spacing())
    };
    for(int m = 0; m < 2; ++m) {
        state.push_back(static_cast<int32_t>(messages_[m].size()));
        for(char c : messages_[m]) {
            state.push_back(static_cast<unsigned char>(c));
        }
    }
    return state;
}

bool BannerMenu::set_saved_state(const std::vector<int32_t>& state)
{
    if(state.size() < 14) return false;
    size_t idx = 0;
    active_message_ = state[idx++] ? 1 : 0;
    external_trigger_enabled_ = state[idx++] != 0;
    scroll_period_ms_ = state[idx++];
    panel_.set_corner(LEDPanel::Corner(state[idx++]));

    colors_[0].set_rgb(state[idx], state[idx+1], state[idx+2], false);
    idx += 3;
    colors_[1].set_rgb(state[idx], state[idx+1], state[idx+2], false);
    idx += 3;

    int mode_val = state[idx++];
    if(mode_val >= 0 && mode_val <= 2) {
        mode_ = static_cast<PlayMode>(mode_val);
    } else {
        mode_ = PlayMode::Repeat;
    }

    int font_val = state[idx++];
    panel_.set_font_spacing(font_val == 0 ? LEDPanel::FontSpacing::Fixed : LEDPanel::FontSpacing::Proportional);

    for(int m = 0; m < 2; ++m) {
        if(idx >= state.size()) return false;
        int len = state[idx++];
        if(len < 0 || idx + len > state.size()) return false;
        messages_[m].clear();
        for(int i = 0; i < len; ++i) {
            messages_[m].push_back(static_cast<char>(state[idx++]));
        }
    }

    set_values(false);
    reset_scroll();
    return true;
}

int32_t BannerMenu::get_version() { return 1; }
int32_t BannerMenu::get_supplier_id() { return 0x42414e52; } // "BANR"
