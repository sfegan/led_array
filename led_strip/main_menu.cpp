
#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/input_menu.hpp"
#include "../common/popup_menu.hpp"

#include "main.hpp"
#include "main_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

#define WRITEVAL(x) \
    { \
        char buffer[80]; \
        sprintf(buffer, "%-20s : %d\n\r", #x, x); \
        puts_raw_nonl(buffer); \
    }

std::vector<SimpleItemValueMenu::MenuItem> MainMenu::make_menu_items() {
    std::vector<SimpleItemValueMenu::MenuItem> menu_items(MIP_NUM_ITEMS);
    menu_items.at(MIP_PIO)         = {"S       : Setup WS2812 device", 0, ""};
    menu_items.at(MIP_MONO_COLOR)  = {"m       : Mono-color menu", 0, ""};
    menu_items.at(MIP_BI_COLOR)    = {"b       : Bi-color menu", 0, ""};
    menu_items.at(MIP_WRITE_STATE) = {"Ctrl-w  : Write state to flash", 0, ""};
    menu_items.at(MIP_REBOOT)      = {"Ctrl-b  : Reboot flasher (press and hold)", 0, ""};
    return menu_items;
}

MainMenu::MainMenu():
    SimpleItemValueMenu(make_menu_items(), std::string("WS2812 pattern generator (Build ")+BuildDate::latest_build_date+")"), 
    pio_(WS2812_DEFAULT_PIN, WS2812_DEFAULT_BAUDRATE),
    mono_color_menu_(pio_),
    bi_color_menu_(pio_, this)
{
    timer_interval_us_ = 1000000; // 1Hz
    add_saved_state_supplier(this);
    add_saved_state_supplier(&pio_);
    add_saved_state_supplier(&mono_color_menu_);
    add_saved_state_supplier(&bi_color_menu_);

    load_state();
}

MainMenu::~MainMenu()
{
    // nothing to see here
}

bool MainMenu::event_loop_starting(int& return_code)
{
    if(selected_menu_ != 0) {
        PopupMenu pm("Starting selected menu, press any key to abort", 5, true, nullptr, "Auto start");
        auto pm_return = pm.event_loop();
        if(pm_return == 0) {
            process_menu_item(selected_menu_);
        }
    }
    return true;
}

void MainMenu::event_loop_finishing(int& return_code)
{
    // nothing to see here
}

bool MainMenu::process_menu_item(int key)
{
    selected_menu_ = key;

    switch(key) {
    case 'S':
        pio_.event_loop();
        break;
        
    case 'm': 
        mono_color_menu_.event_loop();
        break;

    case 'b': 
        bi_color_menu_.event_loop();
        break;

    default:
        selected_menu_ = 0;
        return false;
    }
    
    selected_menu_ = 0;
    return true;
}

bool MainMenu::process_key_press(int key, int key_count, int& return_code,
    const std::vector<std::string>& escape_sequence_parameters, absolute_time_t& next_timer)
{
    if(process_menu_item(key)) {
        this->redraw();
        return true;
    }

    switch(key) {
    case 'D':
    case 'd':
        for(unsigned i=0;i<40;++i) {
            printf("%d: %d\n",i,state()[i]);
        }
        break;

    case 18:
        {
            load_state(true);
            PopupMenu pm("State loaded from flash. Press any key.", 0, true, this, "Information");
            pm.event_loop();
            this->redraw();
        }
        break;

    case 23:
        {
            save_state();
            PopupMenu pm("State written to flash", 2, false, this, "Information");
            pm.event_loop();
            this->redraw();
        }
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

std::vector<int32_t> MainMenu::get_saved_state()
{
    std::vector<int32_t> state;
    state.push_back(selected_menu_);
    return state;
}

bool MainMenu::set_saved_state(const std::vector<int32_t>& state)
{
    if(state.size() != 1) {
        return false;
    }
    selected_menu_ = state[0];
    return true;
}

int32_t MainMenu::get_version()
{
    return 0;
}

int32_t MainMenu::get_supplier_id()
{
    return 0x4e49414d; // "MAIN"
}

int32_t MainMenu::get_application_id()
{
    return 0x17A92635;
}