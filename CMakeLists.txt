cmake_minimum_required(VERSION 3.22)

set(PICO_SDK_PATH "/home/filip/raspberry/pico-sdk")
set(PICO_PLATFORM "rp2350-riscv")

include(pico_sdk_import.cmake)

project(quest C CXX ASM)

pico_sdk_init()

set(CMAKE_CXX_STANDARD 17)

add_executable(quest
        number_input.cpp
        number_input.hpp
        main.cpp
        lcd_control.hpp
)

# if we want to print
pico_enable_stdio_usb(quest 1)

target_link_libraries(quest pico_stdlib hardware_i2c)
pico_add_extra_outputs(quest)
