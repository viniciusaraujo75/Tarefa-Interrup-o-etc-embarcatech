cmake_minimum_required(VERSION 3.13)

# Initialize SDK
include(pico_sdk_import.cmake)

project(main C CXX ASM)

# Set board type (pico_w para WiFi, pico para versão básica)
set(PICO_BOARD pico_w)

# Initialize Pico SDK
pico_sdk_init()

# Configure standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add executable
add_executable(main main.c)

# Configure program metadata
pico_set_program_name(main "BitDogLab_Project")
pico_set_program_version(main "1.0")

# Enable USB output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# Add PIO support (se estiver usando arquivos .pio)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Link libraries
target_link_libraries(main
    pico_stdlib
    hardware_pio
    hardware_timer
    hardware_irq
    hardware_gpio
)

# Add additional includes if needed
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Generate additional output files
pico_add_extra_outputs(main)