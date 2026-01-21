#!/bin/bash
# Build MurmC64 - Commodore 64 emulator (Frodo4) for RP2350
# Copyright (c) 2024-2026 Mikhail Matveev <xtreme@rh1.tech>

rm -rf ./build
mkdir build
cd build
cmake -DPICO_PLATFORM=rp2350 -DCPU_SPEED=252 -DPSRAM_SPEED=133 -DUSB_HID_ENABLED=0 -DDEBUG_LOGS_ENABLED=ON ..
make -j4
