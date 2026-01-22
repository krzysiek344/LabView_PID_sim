cmake_minimum_required(VERSION 3.20.0)

set(BOARD nucleo_f446re)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dc_motor_simulator)

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_sources(app PRIVATE
    src/main.c
    src/motor_simulator.c
    src/uart_handler.c
)




