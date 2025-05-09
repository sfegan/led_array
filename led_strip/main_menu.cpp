
#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/input_menu.hpp"

#include "main.hpp"
#include "main_menu.hpp"
#include "mono_color_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

#define WRITEVAL(x) \
    { \
        char buffer[80]; \
        sprintf(buffer, "%-20s : %d\n\r", #x, x); \
        puts_raw_nonl(buffer); \
    }

void MainMenu::set_pin_value(bool draw)
{ 
    menu_items_[MIP_PIN].value = std::to_string(pio_.pin()); 
    if(draw)draw_item_value(MIP_PIN);
}

void MainMenu::set_baudrate_value(bool draw)
{ 
    menu_items_[MIP_BAUDRATE].value = std::to_string(pio_.baudrate()); 
    if(draw)draw_item_value(MIP_BAUDRATE);
}

std::vector<SimpleItemValueMenu::MenuItem> MainMenu::make_menu_items() {
    std::vector<SimpleItemValueMenu::MenuItem> menu_items(MIP_NUM_ITEMS);
    menu_items.at(MIP_PIN)         = {"P       : Set GPIO pin", 2, "0"};
    menu_items.at(MIP_BAUDRATE)    = {"B       : Set baud rate", 8, "0"};
    menu_items.at(MIP_MONO_COLOR)  = {"m       : Mono-color menu", 0, ""};
    menu_items.at(MIP_BI_COLOR)    = {"b       : Bi-color menu", 0, ""};
    menu_items.at(MIP_REBOOT)      = {"Ctrl-b  : Reboot flasher (press and hold)", 0, ""};
    return menu_items;
}

MainMenu::MainMenu():
    SimpleItemValueMenu(make_menu_items(), std::string("WS2812 pattern generator (Build ")+BuildDate::latest_build_date+")"), 
    pio_(WS2812_DEFAULT_PIN, WS2812_DEFAULT_BAUDRATE)
{
    timer_interval_us_ = 1000000; // 1Hz
    set_pin_value(false);
    set_baudrate_value(false);
}

MainMenu::~MainMenu()
{
    // nothing to see here
}

bool MainMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters, absolute_time_t& next_timer)
{
    switch(key) {
    case 'P':
        {
            int pin = pio_.pin();
            if(InplaceInputMenu::input_value_in_range(pin, 0, 28, this, MIP_PIN, 2)) {
                pio_.set_pin(pin);
            }
            set_pin_value();
        }
        break;
    case 'B':
        {
            int baud = pio_.baudrate();
            if(InplaceInputMenu::input_value_in_range(baud, 0, 10000000, this, MIP_BAUDRATE, 8)) {
                pio_.set_baudrate(baud);
            }
            set_baudrate_value();
        }
        break;
        
    case 'm': 
        {
            // puts("Instantating mono-color menu");
            MonoColorMenu menu(pio_);
            // puts("Starting event loop");
            menu.event_loop();
            this->redraw();
        }
        break;
    case 'b': 
        // {
        //     BiColorMenu menu;
        //     menu.event_loop();
        //     this->redraw();
        // }
        break;
        
    case 7: /* ctrl-g : secret display of menu parameters - to remove */
        cls();
        curpos(1,1);
        WRITEVAL(req_h_);
        WRITEVAL(req_w_);
        WRITEVAL(req_pos_);
        WRITEVAL(screen_h_);
        WRITEVAL(screen_w_);
        WRITEVAL(frame_h_);
        WRITEVAL(frame_w_);
        WRITEVAL(frame_r_);
        WRITEVAL(frame_c_);
        WRITEVAL(item_count_);
        WRITEVAL(item_h_);
        WRITEVAL(item_w_);
        WRITEVAL(val_w_);
        WRITEVAL(item_r_);
        WRITEVAL(item_c_);
        WRITEVAL(val_c_);
        WRITEVAL(item_dr_);
        puts("Press ctrl-L to redraw menu...");
        break;

    default:
        if(key_count==1) {
            beep();
        }
    }
    return true;
}

bool MainMenu::process_timer(bool controller_is_connected, int& return_code, absolute_time_t& next_timer)
{
    if(controller_is_connected) {
        set_heartbeat(!heartbeat_);
    }
    return true;
}
