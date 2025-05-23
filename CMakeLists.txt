cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# include(pico_extras_import_optional.cmake)

project(led_array C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(LED_ARRAY_PATH ${PROJECT_SOURCE_DIR})

# # Initialize the SDK
# if(PICO_BOARD STREQUAL "pico" OR PICO_BOARD STREQUAL "pico_w")
#     message(STATUS "Configuring for Pico: 2MB flash - 4KB reserved")
#     set(PICO_FLASH_SIZE_BYTES 2093056)  # 2MB - 4KB
# elseif(PICO_BOARD STREQUAL "pico2" OR PICO_BOARD STREQUAL "pico_w2")
#     message(STATUS "Configuring for Pico 2: 4MB flash - 4KB reserved")
#     set(PICO_FLASH_SIZE_BYTES 4190208)  # 4MB - 4KB
# else()
#     message(WARNING "Unknown board: defaulting to 2MB flash - 4KB reserved")
#     set(PICO_FLASH_SIZE_BYTES 2093056)
# endif()

pico_sdk_init()


include(led_array_auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

# Hardware-specific examples in subdirectories:
add_subdirectory(common)
add_subdirectory(led_strip)
