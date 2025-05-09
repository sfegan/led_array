#include <string>
#include <vector>
#include <algorithm>

#include <cstring>
#include <cstdio>
#include <cctype>

#include <pico/time.h>
#include <pico/stdlib.h>
#include <pico/stdio.h>
#include <hardware/watchdog.h>

#include "menu.hpp"
#include "build_date.hpp"
#include "reboot_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

int Menu::event_loop(bool enable_escape_sequences, bool enable_reboot)
{
    int return_code = 0;
    if(!this->event_loop_starting(return_code)) {
        return return_code;
    }
    static const int64_t multi_keypress_timeout = 100000; /* 100ms */
    absolute_time_t next_timer = delayed_by_us(get_absolute_time(), timer_interval_us_);
    uint64_t timer_delay = timer_interval_us_;
    bool was_connected = false;
    int last_key = -1;
    absolute_time_t last_key_time = get_absolute_time();
    int key_count = 0;
    bool sent_request_window_size = false;
    std::string escape_sequence;
    std::vector<std::string> escape_sequence_parameters;
    while(true) {
        if(stdio_usb_connected()) {
            if(!was_connected) {
                last_key_time = get_absolute_time();
                if(!this->controller_connected(return_code)) {
                    this->event_loop_finishing(return_code);
                    return return_code;
                }
                if(enable_escape_sequences) {
                    this->send_request_screen_size();
                    sent_request_window_size = true;
                } else {
                    this->redraw();
                }
            }
            was_connected = true;
            int key = getchar_timeout_us(timer_delay);
            absolute_time_t key_time = get_absolute_time();
            if(absolute_time_diff_us(last_key_time, key_time)>multi_keypress_timeout) {
                last_key = -1;
                key_count = 0;
                if(sent_request_window_size) {
                    this->redraw();
                    sent_request_window_size = false;
                }
                if(!escape_sequence.empty()) {
                    for(auto k : escape_sequence) {
                        if(!this->process_key_press(k, 1, return_code, {}, next_timer)) {
                            this->event_loop_finishing(return_code);
                            return return_code;
                        }
                    }
                    escape_sequence.clear();
                    escape_sequence_parameters.clear();
                }
            }
            if(key >= 0) {
                last_key_time = key_time;
                if(!escape_sequence.empty()) {
                    int escaped_key =
                        decode_partial_escape_sequence(key, escape_sequence,    
                            escape_sequence_parameters);
                    switch(escaped_key) {
                    case FAILED_ESCAPE_SEQUENCE:
                        for(auto k : escape_sequence) {
                            if(!this->process_key_press(k, 1, return_code, {}, next_timer)) {
                                this->event_loop_finishing(return_code);
                                return return_code;
                            }
                        }
                        last_key = -1;
                        key_count = 0;
                        escape_sequence.clear();
                        escape_sequence_parameters.clear();
                        break;
                    case INCOMPLETE_ESCAPE_SEQUENCE:
                        break;
                    case UNSUPPORTED_ESCAPE_SEQUENCE:
                        last_key = -1;
                        key_count = 0;
                        escape_sequence.clear();
                        escape_sequence_parameters.clear();
                        break;
                    case CURSOR_POSITION_REPORT:
                        if(sent_request_window_size) {
                            if(escape_sequence_parameters.size() == 2) {
                                int h = std::stoi(escape_sequence_parameters[0]);
                                int w = std::stoi(escape_sequence_parameters[1]);
                                this->set_screen_size(h,w);
                            }                                
                            this->redraw();
                            sent_request_window_size = false;
                        } else {
                            last_key = -1;
                            key_count = 0;
                            if(!this->process_key_press(escaped_key, 1, 
                                return_code, escape_sequence_parameters, next_timer))
                            {
                                this->event_loop_finishing(return_code);
                                return return_code;
                            }
                        }
                        escape_sequence.clear();
                        escape_sequence_parameters.clear();
                        break;
                    default:
                        if(sent_request_window_size) {
                            this->redraw();
                            sent_request_window_size = false;
                        }
                        if(escaped_key == last_key) {
                            ++key_count;
                        } else {
                            last_key = escaped_key;
                            key_count = 1;
                        }
                        if(!this->process_key_press(escaped_key, key_count, 
                            return_code, escape_sequence_parameters, next_timer))
                        {
                            this->event_loop_finishing(return_code);
                            return return_code;
                        }
                        escape_sequence.clear();
                        escape_sequence_parameters.clear();
                        break;
                    }
                } else if(enable_escape_sequences and key == '\033') {
                    // Do no reset last_key and key_count here !!
                    escape_sequence.push_back(key);
                } else if(key == '\014') {
                    last_key = -1;
                    key_count = 0;
                    if(enable_escape_sequences) {
                        this->send_request_screen_size();
                        sent_request_window_size = true;
                    } else {
                        this->redraw();
                    }
                } else if(enable_reboot and key == '\002') {
                    last_key = -1;
                    key_count = 0;
                    RebootMenu reboot(this);
                    reboot.event_loop(/* enable_esc= */ true, /* enable_reboot= */ false);
                    this->redraw();
                } else {
                    if(sent_request_window_size) {
                        this->redraw();
                        sent_request_window_size = false;
                    }
                    if(key == last_key) {
                        ++key_count;
                    } else {
                        last_key = key;
                        key_count = 1;
                    }
                    if(!this->process_key_press(key, key_count, return_code, 
                            escape_sequence_parameters, next_timer)) {
                        this->event_loop_finishing(return_code);
                        return return_code;
                    }                        
                }
                last_key_time = key_time;
            } else { /* timer occurred */
                // nothing happens here anymore
            }
        } else {
            if(was_connected) {
                if(!this->controller_disconnected(return_code)) {
                    this->event_loop_finishing(return_code);
                    return return_code;
                }
                was_connected = false;
                escape_sequence.clear();
                escape_sequence_parameters.clear();
                sent_request_window_size = false;
                last_key = -1;
                key_count = 0;
            }
            sleep_us(1000);
        }

        if(absolute_time_diff_us(get_absolute_time(), next_timer) <= 0) {
            next_timer = delayed_by_us(next_timer, timer_interval_us_);
            if(!this->process_timer(was_connected, return_code, next_timer)) {
                this->event_loop_finishing(return_code);
                return return_code;
            }
        }
        timer_delay = 
            std::max(absolute_time_diff_us(get_absolute_time(), next_timer), 0LL);
    }
    this->event_loop_finishing(return_code);
    return return_code;
}

int Menu::decode_partial_escape_sequence(int key, std::string& escape_sequence,
    std::vector<std::string>& parameters)
{
    int escaped_key = -1;
    int escape_sequence_size = escape_sequence.size();
    if(escape_sequence_size == 1) {
        switch(key) {
        case '\033':
            escaped_key = '\033'; 
            break;
        case '[':
        case 'O':
            escape_sequence.push_back(key);
            escaped_key = INCOMPLETE_ESCAPE_SEQUENCE; 
            break;
        default: 
            escape_sequence.push_back(key);
            escaped_key = FAILED_ESCAPE_SEQUENCE; 
            break;
        }
    } else if(escape_sequence_size==2 and escape_sequence[1] == 'O') {
        switch(key) {
        // See https://www.gnu.org/software/screen/manual/html_node/Input-Translation.html#Input-Translation
        case 'A': escaped_key = KEY_UP; break;
        case 'B': escaped_key = KEY_DOWN; break;
        case 'C': escaped_key = KEY_RIGHT; break;
        case 'D': escaped_key = KEY_LEFT; break;
        case 'H': escaped_key = KEY_HOME; break;
        case 'F': escaped_key = KEY_END; break;
        case 'P': escaped_key = KEY_F1; break;
        case 'Q': escaped_key = KEY_F2; break;
        case 'R': escaped_key = KEY_F3; break;
        case 'S': escaped_key = KEY_F4; break;
        case 'p': escaped_key = '0'; break;
        case 'q': escaped_key = '1'; break;
        case 'r': escaped_key = '2'; break;
        case 's': escaped_key = '3'; break;
        case 't': escaped_key = '4'; break;
        case 'u': escaped_key = '5'; break;
        case 'v': escaped_key = '6'; break;
        case 'w': escaped_key = '7'; break;
        case 'x': escaped_key = '8'; break;
        case 'y': escaped_key = '9'; break;
        case 'k': escaped_key = '+'; break;
        case 'm': escaped_key = '-'; break;
        case 'j': escaped_key = '*'; break;
        case 'o': escaped_key = '/'; break;
        case 'X': escaped_key = '='; break;
        case 'n': escaped_key = '.'; break;
        case 'l': escaped_key = ','; break;
        case 'M': escaped_key = '\r'; break;
        default: 
            escape_sequence.push_back(key);
            escaped_key = UNSUPPORTED_ESCAPE_SEQUENCE; 
            break;
        }
    } else if(escape_sequence[1] == '[') {
        switch(key)
        {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            if(parameters.empty())parameters.push_back({});
            parameters.back().push_back(key);
            escape_sequence.push_back(key);
            escaped_key = INCOMPLETE_ESCAPE_SEQUENCE;
            break;
        case ';':
            parameters.push_back({});
            escape_sequence.push_back(key);
            escaped_key = INCOMPLETE_ESCAPE_SEQUENCE;
            break;
        case ':': case '<': case '=': case '>': case '?': 
        case ' ': case '!': case '"': case '#': case '$': case '%':
        case '&': case '\'': case '(': case ')': case '*': case '+':
        case ',': case '-': case '.': case '/':
            escape_sequence.push_back(key);
            escaped_key = INCOMPLETE_ESCAPE_SEQUENCE;
            break;
        case 'A': escaped_key = KEY_UP; break;
        case 'B': escaped_key = KEY_DOWN; break;
        case 'C': escaped_key = KEY_RIGHT; break;
        case 'D': escaped_key = KEY_LEFT; break;
        case 'F': escaped_key = KEY_END; break;
        case 'H': escaped_key = KEY_HOME; break;
        case 'R': escaped_key = CURSOR_POSITION_REPORT; break;
        case '~':
            if(escape_sequence_size == 3) {
                switch(escape_sequence[2]) 
                {
                case '1': escaped_key = KEY_HOME; break;
                case '2': escaped_key = KEY_INSERT; break;
                case '3': escaped_key = KEY_DELETE; break;
                case '4': escaped_key = KEY_END; break;
                case '5': escaped_key = KEY_PAGE_UP; break;
                case '6': escaped_key = KEY_PAGE_DOWN; break;
                case '7': escaped_key = KEY_HOME; break;
                case '8': escaped_key = KEY_END; break;
                default: 
                    escape_sequence.push_back(key);
                    escaped_key = UNSUPPORTED_ESCAPE_SEQUENCE; break;
                    break;
                }
            } else if(escape_sequence_size==4 and escape_sequence[2]=='1') {
                switch(escape_sequence[3]) 
                {
                case '0': escaped_key = KEY_F0; break;
                case '1': escaped_key = KEY_F1; break;
                case '2': escaped_key = KEY_F2; break;
                case '3': escaped_key = KEY_F3; break;
                case '4': escaped_key = KEY_F4; break;
                case '5': escaped_key = KEY_F5; break;
                case '7': escaped_key = KEY_F6; break;
                case '8': escaped_key = KEY_F7; break;
                case '9': escaped_key = KEY_F8; break;
                default: 
                    escape_sequence.push_back(key);
                    escaped_key = UNSUPPORTED_ESCAPE_SEQUENCE; break;
                    break;
                }
            } else if(escape_sequence_size==4 and escape_sequence[2]=='2') {
                switch(escape_sequence[3]) 
                {
                case '0': escaped_key = KEY_F9; break;
                case '1': escaped_key = KEY_F10; break;
                case '3': escaped_key = KEY_F11; break;
                case '4': escaped_key = KEY_F12; break;
                default: 
                    escape_sequence.push_back(key);
                    escaped_key = UNSUPPORTED_ESCAPE_SEQUENCE; break;
                    break;
                }
            } else {
                escape_sequence.push_back(key);
                escaped_key = UNSUPPORTED_ESCAPE_SEQUENCE; break;
            }
            break;
        default:
            escape_sequence.push_back(key);
            if(key >= 0x40 and key <= 0x7F) {
                escaped_key = UNSUPPORTED_ESCAPE_SEQUENCE;
            } else {
                escaped_key = FAILED_ESCAPE_SEQUENCE;
            }
            break;
        }
    } else {
        escape_sequence.push_back(key);
        escaped_key = FAILED_ESCAPE_SEQUENCE;
    }
    return escaped_key;
}
