add_library(lsp_common STATIC build_date.cpp input_menu.cpp reboot_menu.cpp
        menu_event_loop.cpp menu.cpp)

pico_generate_pio_header(lsp_common ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)


target_link_libraries(lsp_common pico_stdlib pico_multicore pico_sync hardware_pio hardware_adc)

pico_enable_stdio_usb(lsp_common 1)
pico_enable_stdio_uart(lsp_common 0)
