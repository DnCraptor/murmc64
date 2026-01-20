cmake_minimum_required(VERSION 3.13)

# Set board to Pico 2 (RP2350)
set(PICO_BOARD pico2 CACHE STRING "Pico board type")

# Board configuration variant (M1, M2)
set(BOARD_VARIANT "M1" CACHE STRING "Board variant: M1 or M2")

# CPU overclock configuration (252, 378, 504 MHz)
set(CPU_SPEED "252" CACHE STRING "CPU clock in MHz: 252, 378, 504")

# PSRAM speed configuration (84, 100, 133, 166 MHz target)
set(PSRAM_SPEED "133" CACHE STRING "PSRAM max frequency in MHz: 84, 100, 133, 166")

# CPU voltage selection based on speed
if(CPU_SPEED GREATER_EQUAL 504)
    set(CPU_VOLTAGE "VREG_VOLTAGE_1_65")
elseif(CPU_SPEED GREATER_EQUAL 378)
    set(CPU_VOLTAGE "VREG_VOLTAGE_1_60")
else()
    set(CPU_VOLTAGE "VREG_VOLTAGE_1_50")
endif()

# I2S Audio pin configuration based on board variant
if(BOARD_VARIANT STREQUAL "M1")
    set(I2S_DATA_PIN "26")
    set(I2S_CLOCK_PIN_BASE "27")
else()  # M2
    set(I2S_DATA_PIN "9")
    set(I2S_CLOCK_PIN_BASE "10")
endif()

# USB HID Host support (keyboard and gamepad via USB)
# When enabled, USB CDC stdio is disabled!
option(USB_HID_ENABLED "Enable USB HID Host for keyboard/gamepad" OFF)

# PS/2 keyboard support toggle
option(PS2_KEYBOARD_ENABLED "Enable PS/2 keyboard input" ON)

# Verbose debug logging toggle
option(DEBUG_LOGS_ENABLED "Enable verbose debug logging" OFF)

# Use Frodo Lite (line-based) instead of Frodo SC (cycle-accurate) for better performance
option(FRODO_LITE "Use Frodo Lite (line-based emulation)" ON)

message(STATUS "murmfrodo4 - Commodore 64 Emulator (Frodo4) for RP2350")
message(STATUS "Board: ${BOARD_VARIANT}, CPU: ${CPU_SPEED} MHz, PSRAM: ${PSRAM_SPEED} MHz, Voltage: ${CPU_VOLTAGE}")
message(STATUS "I2S Audio: DATA=${I2S_DATA_PIN}, CLK_BASE=${I2S_CLOCK_PIN_BASE}")

include(pico_sdk_import.cmake)

# Import pico-extras for audio_i2s library
set(PICO_EXTRAS_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-extras CACHE PATH "Path to pico-extras")
if(EXISTS ${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
    include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
endif()

project(murmfrodo4 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

# Initialize pico-extras if available
if(COMMAND pico_extras_init)
    pico_extras_init()
endif()

# Copy driver subdirectories from murmapple structure
add_subdirectory(drivers/ps2kbd)
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/usbhid)

# Create a library for drivers
add_library(drivers
    drivers/HDMI.c
    drivers/psram_init.c
    drivers/psram_allocator.c
    drivers/nespad/nespad.c
)
target_include_directories(drivers PUBLIC drivers drivers/nespad src)
target_compile_definitions(drivers PRIVATE
    BOARD_${BOARD_VARIANT}
    PSRAM_MAX_FREQ_MHZ=${PSRAM_SPEED}
)
target_link_libraries(drivers pico_stdlib hardware_dma hardware_pio hardware_spi)

# Frodo4 C64 Emulator sources
add_executable(murmfrodo4
    # RP2350 Platform layer
    src/rp2350/main_rp2350.c
    src/rp2350/Display_rp2350.cpp
    src/rp2350/C64_rp2350.cpp
    src/rp2350/Prefs_rp2350.cpp
    src/rp2350/ROM_data.cpp
    src/rp2350/sid_i2s.cpp
    src/rp2350/SID_rp2350.cpp
    src/rp2350/input_rp2350.cpp
    src/rp2350/disk_loader.c
    src/rp2350/disk_ui.c
    src/rp2350/startscreen.c
    src/rp2350/fatfs_stdio.c

    # Frodo4 emulator core (C++) - full emulation
    src/CPUC64.cpp
    src/VIC.cpp
    # src/SID.cpp  # Replaced by SID_rp2350.cpp (uses I2S instead of SDL)
    src/CIA.cpp
    src/IEC.cpp
    src/Cartridge.cpp
    src/REU.cpp
    src/rp2350/Tape_stub.cpp  # Stub implementation for RP2350
    src/1541d64.cpp
    # src/1541t64.cpp  # Disabled for RP2350 (T64 tape archives not supported)
    src/1541gcr.cpp
    # src/1541fs.cpp   # Disabled for RP2350 (filesystem drive not supported)
    src/CPU1541.cpp
    src/VIA.cpp
)

# Frodo Lite (line-based) vs Frodo SC (cycle-accurate) emulation
if(FRODO_LITE)
    message(STATUS "Using Frodo Lite (line-based emulation)")
else()
    message(STATUS "Using Frodo SC (subcycle-accurate emulation)")
    target_compile_definitions(murmfrodo4 PRIVATE FRODO_SC=1)
endif()

target_include_directories(murmfrodo4 PRIVATE
    src
    src/rp2350
    drivers
)

target_compile_definitions(murmfrodo4 PRIVATE
    BOARD_${BOARD_VARIANT}
    CPU_CLOCK_MHZ=${CPU_SPEED}
    CPU_VOLTAGE=${CPU_VOLTAGE}
    PSRAM_MAX_FREQ_MHZ=${PSRAM_SPEED}
    # RP2350 platform flag
    FRODO_RP2350=1
    # Disable SDL and GTK
    # I2S Audio configuration
    # NOTE: HDMI uses PIO1, PS/2 uses PIO0 SM0 (claimed dynamically)
    # So I2S uses PIO0 SM2 (claimed statically before PS/2 init)
    PICO_AUDIO_I2S_PIO=0
    PICO_AUDIO_I2S_STATE_MACHINE=2
    PICO_AUDIO_I2S_DMA_IRQ=1
    PICO_AUDIO_I2S_DMA_CHANNEL=6
    PICO_AUDIO_I2S_DATA_PIN=${I2S_DATA_PIN}
    PICO_AUDIO_I2S_CLOCK_PIN_BASE=${I2S_CLOCK_PIN_BASE}
    FEATURE_AUDIO=1
)

if(PS2_KEYBOARD_ENABLED)
    target_compile_definitions(murmfrodo4 PRIVATE ENABLE_PS2_KEYBOARD=1)
else()
    target_compile_definitions(murmfrodo4 PRIVATE ENABLE_PS2_KEYBOARD=0)
endif()

if(DEBUG_LOGS_ENABLED)
    target_compile_definitions(murmfrodo4 PRIVATE ENABLE_DEBUG_LOGS=1)
else()
    target_compile_definitions(murmfrodo4 PRIVATE ENABLE_DEBUG_LOGS=0)
endif()

# Optimization for maximum performance on RP2350
# -O3: Maximum optimization including loop vectorization
# -ffunction-sections -fdata-sections: Allow linker to remove unused code
# -fno-strict-aliasing: Allow type punning (common in emulators)
target_compile_options(murmfrodo4 PRIVATE
    -O3
    -ffunction-sections
    -fdata-sections
    -fno-strict-aliasing
    -fno-exceptions
    -fno-rtti
)

target_link_libraries(murmfrodo4
    pico_stdlib
    pico_multicore
    pico_audio_i2s
    hardware_dma
    hardware_pio
    hardware_clocks
    hardware_vreg
    hardware_spi
    drivers
    ps2kbd
    sdcard
    fatfs
    usbhid
)

# USB CDC stdio: enabled for debugging, but disabled when USB HID is enabled
# (USB port can only be Host OR Device, not both)
if(USB_HID_ENABLED)
    pico_enable_stdio_usb(murmfrodo4 0)
    message(STATUS "USB CDC stdio disabled (USB HID Host is enabled)")
else()
    pico_enable_stdio_usb(murmfrodo4 1)
    # NOTE: Do NOT explicitly link tinyusb_device - pico_enable_stdio_usb handles it
    # Explicit linking causes "CDC not enabled" warning and breaks USB Serial!
    message(STATUS "USB CDC stdio ENABLED (USB HID Host is disabled)")
endif()
pico_enable_stdio_uart(murmfrodo4 1)

# Create UF2 output
pico_add_extra_outputs(murmfrodo4)
