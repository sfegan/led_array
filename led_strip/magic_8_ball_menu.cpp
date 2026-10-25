#include "magic_8_ball_menu.hpp"

#include <algorithm>

#include <hardware/gpio.h>

#include "../common/input_menu.hpp"
#include "../common/popup_menu.hpp"

Magic8BallMenu* Magic8BallMenu::active_menu_ = nullptr;

namespace {
struct Answer { const char* text; int category; }; // 0 positive, 1 uncertain, 2 negative
constexpr Answer answers[] = {
    {"IT IS CERTAIN", 0}, {"IT IS DECIDEDLY SO", 0}, {"WITHOUT A DOUBT", 0},
    {"YES DEFINITELY", 0}, {"YOU MAY RELY ON IT", 0}, {"AS I SEE IT YES", 0},
    {"MOST LIKELY", 0}, {"OUTLOOK GOOD", 0}, {"YES", 0}, {"SIGNS POINT TO YES", 0},
    {"REPLY HAZY TRY AGAIN", 1}, {"ASK AGAIN LATER", 1}, {"BETTER NOT TELL YOU NOW", 1},
    {"CANNOT PREDICT NOW", 1}, {"CONCENTRATE AND ASK AGAIN", 1},
    {"DO NOT COUNT ON IT", 2}, {"MY REPLY IS NO", 2}, {"MY SOURCES SAY NO", 2},
    {"OUTLOOK NOT SO GOOD", 2}, {"VERY DOUBTFUL", 2},
};
}

Magic8BallMenu::Magic8BallMenu(SerialPIO& pio, SavedStateManager* saved_state_manager):
    SimpleItemValueMenu(make_menu_items(), "Magic 8-ball"), pio_(pio),
    saved_state_manager_(saved_state_manager), panel_(32, 8),
    colors_{
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V),
        RGBHSVMenuItems(*this, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V) }
{
    timer_interval_us_ = 10000; // 100 Hz: smooth enough for animation and button handling
    colors_[ShimmerA].set_rgb(24, 0, 80, false);
    colors_[ShimmerB].set_rgb(0, 12, 64, false);
    colors_[Positive].set_rgb(0, 96, 0, false);
    colors_[Uncertain].set_rgb(96, 64, 0, false);
    colors_[Negative].set_rgb(96, 0, 0, false);
    colors_[Cursor].set_rgb(32, 32, 32, false);
    set_values(false);
}

std::vector<SimpleItemValueMenu::MenuItem> Magic8BallMenu::make_menu_items()
{
    std::vector<MenuItem> items(MIP_NUM_ITEMS);
    items[MIP_COLOR] = {"c/C     : Cycle modifiable colour", 10, "SHIMMER A"};
    RGBHSVMenuItems::make_menu_items(items, MIP_R, MIP_G, MIP_B, MIP_H, MIP_S, MIP_V);
    items[MIP_THINK_TIME] = {"T       : Thinking time [tenths of sec]", 3, "30"};
    items[MIP_SCROLL_SPEED] = {"x       : Pixel scroll period [ms]", 3, "50"};
    items[MIP_CORNER] = {"o/O     : Cycle LED-0 panel corner (serpentine)", 12, "TOP LEFT"};
    items[MIP_CURSOR_ROWS] = {"k       : Idle cursor height [rows]. 0=OFF, negative=above", 3, "8"};
    items[MIP_PROMPT] = {"p       : Set idle prompt", 4, ""};
    items[MIP_TRIGGER_NOW] = {"Space   : Trigger Magic 8-ball", 0, ""};
    // Refined at mode start once the active LED count establishes the panel width.
    items[MIP_STATE] = {"        : Current state", 10, "IDLE"};
    items[MIP_WRITE_STATE] = {"Ctrl-w  : Write state to flash", 0, ""};
    items[MIP_EXIT] = {"q       : Exit menu", 0, ""};
    return items;
}

const char* Magic8BallMenu::color_set_name() const
{
    static constexpr const char* names[] = {"SHIMMER A", "SHIMMER B", "POSITIVE", "UNCERTAIN", "NEGATIVE", "CURSOR"};
    return names[color_set_];
}

const char* Magic8BallMenu::corner_name() const
{
    switch(panel_.corner()) {
    case LEDPanel::Corner::TopLeft: return "TOP LEFT";
    case LEDPanel::Corner::TopRight: return "TOP RIGHT";
    case LEDPanel::Corner::BottomLeft: return "BOTTOM LEFT";
    case LEDPanel::Corner::BottomRight: return "BOTTOM RIGHT";
    }
    return "TOP LEFT";
}

uint32_t Magic8BallMenu::color(ColorSet set) const
{
    return rgb_to_grbz(colors_[set].r(), colors_[set].g(), colors_[set].b());
}

void Magic8BallMenu::set_color_set(bool draw)
{
    menu_items_[MIP_COLOR].value = color_set_name();
    if(draw) draw_item_value(MIP_COLOR);
    colors_[color_set_].redraw(draw);
}

void Magic8BallMenu::set_values(bool draw)
{
    set_color_set(draw);
    menu_items_[MIP_THINK_TIME].value = std::to_string(think_time_tenths_);
    menu_items_[MIP_SCROLL_SPEED].value = std::to_string(scroll_period_ms_);
    menu_items_[MIP_CORNER].value = corner_name();
    if(cursor_rows_==0) {
        menu_items_[MIP_CURSOR_ROWS].set_onoff(false);
    } else {
        menu_items_[MIP_CURSOR_ROWS].value = std::to_string(cursor_rows_);
    }
    menu_items_[MIP_PROMPT].value = prompt_;
    set_state_value(false);
    if(draw) for(int i = MIP_THINK_TIME; i <= MIP_PROMPT; ++i) draw_item_value(i);
    if(draw) draw_item_value(MIP_STATE);
}

int Magic8BallMenu::max_prompt_length() const
{
    return panel_.width() / 8;
}

std::string Magic8BallMenu::state_value() const
{
    if(state_ == State::Idle) return "IDLE";
    if(state_ == State::Thinking) {
        if(forced_response_ == 1) return "THINKING+";
        if(forced_response_ == 2) return "THINKING-";
        return "THINKING";
    }

    const std::string text = answers[response_index_].text;
    // Report the text currently crossing the window, rather than just a
    // generic state.  Each glyph occupies eight pixels plus a one-pixel gap.
    const int first = std::max(0, -scroll_x_ / 9);
    const int last = std::min(int(text.size()), (panel_.width() - scroll_x_ + 8) / 9);
    if(first >= last) return scroll_x_ >= 0 ? "\"ENTERING\"" : "\"LEAVING\"";
    const std::string visible = text.substr(first, last - first);
    return "\"" + visible + "\"";
}

void Magic8BallMenu::set_state_value(bool draw)
{
    const std::string value = state_value();
    const bool changed = value != last_state_value_;
    menu_items_[MIP_STATE].value = value;
    last_state_value_ = value;
    // Do not redraw the terminal 100 times per second while an unchanged
    // idle/thinking state is displayed.
    if(draw && changed) draw_item_value(MIP_STATE);
}

void Magic8BallMenu::configure_trigger_pin()
{
    const auto configure = [](int pin) {
        if(pin < 0) return;
        gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, false);
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &Magic8BallMenu::gpio_callback);
    };
    configure(pio_.trigger_line_1());
    if(pio_.trigger_line_2() != pio_.trigger_line_1()) configure(pio_.trigger_line_2());
}

void Magic8BallMenu::gpio_callback(uint gpio, uint32_t events)
{
    if(active_menu_ && (gpio == unsigned(active_menu_->pio_.trigger_line_1()) ||
        (active_menu_->pio_.trigger_line_2() >= 0 && gpio == unsigned(active_menu_->pio_.trigger_line_2()))) &&
        (events & GPIO_IRQ_EDGE_FALL)) {
        const uint64_t now = time_us_64();
        if(now - active_menu_->last_trigger_us_ >= 100000) { // switch debounce
            active_menu_->last_trigger_us_ = now;
            active_menu_->handle_trigger();
        }
    }
}

void Magic8BallMenu::start_thinking()
{
    state_ = State::Thinking;
    think_ticks_ = 0;
    forced_response_ = 0;
}

void Magic8BallMenu::handle_trigger()
{
    if(state_ == State::Idle) start_thinking();
    else if(state_ == State::Thinking && forced_response_ < 2) ++forced_response_;
}

void Magic8BallMenu::choose_response()
{
    std::vector<int> choices;
    for(unsigned i = 0; i < sizeof(answers) / sizeof(answers[0]); ++i) {
        // The first extra trigger forces positive (category 0); the second
        // forces negative (category 2), deliberately skipping uncertain.
        const int forced_category = forced_response_ == 1 ? 0 : 2;
        if(forced_response_ == 0 || answers[i].category == forced_category) choices.push_back(i);
    }
    response_index_ = choices[rng_() % choices.size()];
    scroll_x_ = panel_.width();
    state_ = State::Scrolling;
}

void Magic8BallMenu::send_panel()
{
    panel_.clear();
    if(state_ == State::Idle) {
        panel_.draw_text(prompt_, 0, color(Cursor), 0);
        if(cursor_rows_!=0 && ((frame_ / 50) & 1) == 0) {
            const int cursor_cell = std::min(int(prompt_.size()), max_prompt_length() - 1);
            if(cursor_rows_ < 0) {
                panel_.fill_rect(cursor_cell * 8, 0, 8, -cursor_rows_, color(Cursor));
            } else {
                panel_.fill_rect(cursor_cell * 8, panel_.height() - cursor_rows_, 8, cursor_rows_, color(Cursor));
            }
        }
    } else if(state_ == State::Thinking) {
        // A low-current, deterministic shimmer: each pixel alternates between
        // the two configurable colours, with a diagonally moving phase.
        for(int y = 0; y < panel_.height(); ++y)
            for(int x = 0; x < panel_.width(); ++x)
                panel_.set_pixel(x, y, ((x + y * 3 + int(frame_ / 3)) & 7) < 3 ? color(ShimmerA) : color(ShimmerB));
    } else {
        const Answer& answer = answers[response_index_];
        const ColorSet sets[] = {Positive, Uncertain, Negative};
        panel_.draw_text(answer.text, scroll_x_, color(sets[answer.category]));
    }

    std::vector<uint32_t> output(std::max(0, pio_.nled()), 0);
    const int active_start = pio_.back() ? pio_.nled() - pio_.non() : 0;
    const int copied = std::min({int(panel_.pixels().size()), pio_.non(), pio_.nled()});
    for(int i = 0; i < copied; ++i) output[active_start + i] = panel_.pixels()[i];
    pio_.put_pixel_vector(output);
    pio_.flush();
}

bool Magic8BallMenu::event_loop_starting(int&)
{
    // Text is always eight pixels high.  Active LEDs therefore determine the
    // number of horizontal columns: 256 = 32x8, 512 = 64x8, and so on.
    if(pio_.non() >= 8 && pio_.non() % 8 == 0) panel_.resize(pio_.non() / 8, 8);
    if(int(prompt_.size()) > max_prompt_length()) prompt_.resize(max_prompt_length());
    // A visible glyph consumes nine columns (eight pixels plus its gap).
    // Quotes take two more characters; retain space for the status words too.
    menu_items_[MIP_STATE].max_value_size = std::max(10, (panel_.width() + 8) / 9 + 2);
    menu_items_[MIP_PROMPT].max_value_size = max_prompt_length();
    active_menu_ = this;
    configure_trigger_pin();
    pio_.activate_program();
    set_state_value(false);
    send_panel();
    return true;
}

void Magic8BallMenu::event_loop_finishing(int&)
{
    gpio_set_irq_enabled(pio_.trigger_line_1(), GPIO_IRQ_EDGE_FALL, false);
    if(pio_.trigger_line_2() >= 0 && pio_.trigger_line_2() != pio_.trigger_line_1())
        gpio_set_irq_enabled(pio_.trigger_line_2(), GPIO_IRQ_EDGE_FALL, false);
    active_menu_ = nullptr;
    pio_.put_pixel(0, pio_.nled());
    pio_.flush();
    pio_.deactivate_program();
}

bool Magic8BallMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>&, absolute_time_t&)
{
    bool changed = false;
    if(colors_[color_set_].process_key_press(key, key_count, changed)) {
        if(changed) send_panel();
        return true;
    }
    switch(key) {
    case ' ': handle_trigger(); break;
    case 'c': color_set_ = ColorSet((color_set_ + 1) % ColorSetCount); set_color_set(); break;
    case 'C': color_set_ = ColorSet((color_set_ + ColorSetCount - 1) % ColorSetCount); set_color_set(); break;
    case 'T': InplaceInputMenu::input_value_in_range(think_time_tenths_, 1, 255, this, MIP_THINK_TIME, 3); menu_items_[MIP_THINK_TIME].value = std::to_string(think_time_tenths_); draw_item_value(MIP_THINK_TIME); break;
    case 'x': InplaceInputMenu::input_value_in_range(scroll_period_ms_, 10, 255, this, MIP_SCROLL_SPEED, 3); menu_items_[MIP_SCROLL_SPEED].value = std::to_string(scroll_period_ms_); draw_item_value(MIP_SCROLL_SPEED); break;
    case 'o': panel_.set_corner(LEDPanel::Corner((int(panel_.corner()) + 1) % 4)); menu_items_[MIP_CORNER].value = corner_name(); draw_item_value(MIP_CORNER); send_panel(); break;
    case 'O': panel_.set_corner(LEDPanel::Corner((int(panel_.corner()) + 3) % 4)); menu_items_[MIP_CORNER].value = corner_name(); draw_item_value(MIP_CORNER); send_panel(); break;
    case 'k': 
        if(cursor_rows_==0) {
            menu_items_[MIP_CURSOR_ROWS].value = std::to_string(cursor_rows_);
            draw_item_value(MIP_CURSOR_ROWS); 
        }
        InplaceInputMenu::input_value_in_range(cursor_rows_, -7, 8, this, MIP_CURSOR_ROWS, 2); 
        if(cursor_rows_==0) {
            menu_items_[MIP_CURSOR_ROWS].set_onoff(false);
        } else {
            menu_items_[MIP_CURSOR_ROWS].value = std::to_string(cursor_rows_);
        }
        draw_item_value(MIP_CURSOR_ROWS); 
        send_panel(); 
        break;
    case 'p': {
        // Keep editing in the menu's value field, so there is no separate
        // dialog to clean up and the prompt remains visible as it is typed.
        SimpleItemValueRowAndColumnGetter rc_getter(this, MIP_PROMPT);
        InplaceInputMenu input(rc_getter, max_prompt_length(), VI_STRING, true, this);
        input.set_value(prompt_);
        if(input.event_loop() == 1) {
            prompt_ = input.get_value();
            menu_items_[MIP_PROMPT].value = prompt_;
            send_panel();
        }
        // Replace the in-place cursor/underscore rendering with a clean menu.
        redraw();
        break;
    }
    case 23: if(saved_state_manager_) { saved_state_manager_->save_state(); PopupMenu pm("State written to flash", 2, true, this, "Information"); pm.event_loop(); redraw(); } break;
    case 'q': case 'Q': return_code = 0; return false;
    default: if(key_count == 1) beep(); break;
    }
    return true;
}

bool Magic8BallMenu::process_timer(bool controller_is_connected, int&, absolute_time_t&)
{
    ++frame_;
    if(state_ == State::Thinking && ++think_ticks_ >= think_time_tenths_ * 10) choose_response();
    if(state_ == State::Scrolling && frame_ % std::max(1, scroll_period_ms_ / 10) == 0) {
        if(--scroll_x_ < -panel_.text_width(answers[response_index_].text)) state_ = State::Idle;
    }
    if(controller_is_connected && frame_ % 100 == 0) set_heartbeat(!heartbeat_);
    set_state_value(true);
    send_panel();
    return true;
}

std::vector<int32_t> Magic8BallMenu::get_saved_state()
{
    std::vector<int32_t> state = {think_time_tenths_, scroll_period_ms_, int(panel_.corner()), cursor_rows_!=0, cursor_rows_, int(color_set_)};
    for(auto& c : colors_) { state.push_back(c.r()); state.push_back(c.g()); state.push_back(c.b()); }
    state.push_back(prompt_.size());
    for(char c : prompt_) state.push_back(static_cast<unsigned char>(c));
    return state;
}

bool Magic8BallMenu::set_saved_state(const std::vector<int32_t>& state)
{
    if(state.size() < 25) return false;
    think_time_tenths_ = state[0]; scroll_period_ms_ = state[1];
    panel_.set_corner(LEDPanel::Corner(state[2])); cursor_rows_ = state[4]; color_set_ = ColorSet(state[5]);
    for(int i = 0; i < ColorSetCount; ++i) colors_[i].set_rgb(state[6 + i * 3], state[7 + i * 3], state[8 + i * 3], false);
    const int prompt_size = state[24];
    if(prompt_size < 0 || state.size() != unsigned(25 + prompt_size)) return false;
    prompt_.clear();
    for(int i = 0; i < prompt_size; ++i) prompt_.push_back(static_cast<char>(state[25 + i]));
    set_values(false);
    return true;
}

int32_t Magic8BallMenu::get_version() { return 2; }
int32_t Magic8BallMenu::get_supplier_id() { return 0x4d38424c; } // "M8BL"
