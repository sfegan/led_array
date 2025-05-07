add_executable(led_strip)

# pico_generate_pio_header(flasher ${CMAKE_CURRENT_LIST_DIR}/set_charges.pio)

target_sources(led_strip PRIVATE main.cpp 
        main_menu.cpp
        mono_color_menu.cpp)

# pull in common dependencies
target_link_libraries(led_strip PRIVATE
        lsp_common pico_stdlib pico_multicore pico_sync hardware_pio hardware_adc)
target_compile_definitions(led_strip PRIVATE)

# create map/bin/hex file etc.
pico_add_extra_outputs(led_strip)

# add url via pico_set_program_url
led_array_auto_set_url(led_strip)

pico_enable_stdio_usb(led_strip 1)
pico_enable_stdio_uart(led_strip 0)

