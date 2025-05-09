#include <string>
#include <vector>

#include <cstring>
#include <cstdio>
#include <cctype>

#include <pico/stdlib.h>
#include <hardware/adc.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>


#include "../common/build_date.hpp"
#include "../common/menu.hpp"
#include "../common/ws2812.pio.h"
#include "main_menu.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

#define WS2812_PIN 2

PIO main_pio;
uint main_sm;

int main()
{
    uint offset;
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &main_pio, &main_sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(main_pio, main_sm, offset, WS2812_PIN, 800000, false);


    // gpio_init(PICO_DEFAULT_LED_PIN);
    // gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    // gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // adc_init();
    // adc_set_temp_sensor_enabled(true);
    // adc_select_input(4);

    // uint32_t pin_mask =
    //     (0xFFU << VDAC_BASE_PIN)
    //     | (0xFU << ROW_A_BASE_PIN)
    //     | (0xFU << COL_A_BASE_PIN)
    //     | (0x1U << DAC_EN_PIN)
    //     | (0x1U << TRIG_PIN)
    //     | (0x1U << SPI_CLK_PIN)
    //     | (0x1U << SPI_DOUT_PIN)
    //     | (0x1U << SPI_COL_EN_PIN)
    //     | (0x1U << SPI_ALL_EN_PIN)
    //     | (0x1U << DAC_WR_PIN)
    //     | (0x3U << DAC_SEL_BASE_PIN);

    // gpio_init_mask(pin_mask);
    // gpio_clr_mask(pin_mask);
    // gpio_set_dir_out_masked(pin_mask);

    stdio_init_all();

    // EventDispatcher::instance().start_dispatcher();

    MainMenu menu;
    // SingleLEDEventGenerator menu;
    // EventDispatcher::instance().register_event_generator(&menu);
    menu.event_loop();
}
