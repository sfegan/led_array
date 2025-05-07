#pragma once

#include <string>

#include "menu.hpp"

enum ValidInput { VI_STRING, VI_FLOAT, VI_POSITIVE_FLOAT, VI_INTEGER, VI_NATURAL };
 
class InplaceInputMenu: public Menu {
public:
    InplaceInputMenu(RowAndColumnGetter& row_col_getter, unsigned max_value_size, 
        ValidInput valid_input=VI_STRING, bool do_highlight=true, Menu* base_menu = nullptr);
    InplaceInputMenu(int r, int c, unsigned max_value_size, ValidInput valid_input=VI_STRING, 
        bool do_highlight=true, Menu* base_menu = nullptr, RowAndColumnGetter* row_col_getter = nullptr);
    virtual ~InplaceInputMenu();
    void redraw() override;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters,
        absolute_time_t& next_timer) override;
    bool process_timer(bool controller_is_connected, int& return_code,
        absolute_time_t& next_timer) override;
    const std::string& get_value() const { return value_; }
    void cancelled();

    static bool input_value_in_range(int& value, int value_min, int value_max,
        SimpleItemValueMenu* base_menu, int iitem, unsigned max_value_size = 0);
        
private:
    void draw_value();
    bool is_valid(int key);

    Menu* base_menu_ = nullptr;
    RowAndColumnGetter* row_col_getter_ = nullptr;
    int r_;
    int c_;
    unsigned max_value_size_;
    ValidInput valid_input_ = VI_STRING;
    std::string value_;
    bool do_highlight_;
    bool blink_on_ = false;
    bool first_redraw_ = true;
};

class InputMenu: public FramedMenu, RowAndColumnGetter {
public:
    InputMenu(unsigned max_value_size, ValidInput valid_input=VI_STRING, const std::string title = "Enter value", 
        const std::string prompt = "Enter value: ", Menu* base_menu = nullptr);
    InputMenu(unsigned max_value_size, const std::string title,
        const std::string prompt = "Enter value: ", ValidInput valid_input=VI_STRING, Menu* base_menu = nullptr);
    virtual ~InputMenu();
    void redraw() override;
    bool event_loop_starting(int& return_code) override;
    void event_loop_finishing(int& return_code) override ;
    bool controller_connected(int& return_code) override;
    bool controller_disconnected(int& return_code) override;
    bool process_key_press(int key, int key_count, int& return_code,
        const std::vector<std::string>& escape_sequence_parameters, 
        absolute_time_t& next_timer) override;
    bool process_timer(bool controller_is_connected, int& return_code,
        absolute_time_t& next_timer) override;
    const std::string& get_value() const;
    void cancelled();
    int row() override;
    int col() override;
private:
    InplaceInputMenu iim_;
    Menu* base_menu_ = nullptr;
    std::string prompt_;
    bool first_redraw_ = true;
};
