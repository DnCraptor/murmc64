#!/bin/bash
# Build murmfrodo4 - Commodore 64 emulator (Frodo4) for RP2350

rm -rf ./build
mkdir build
cd build
cmake -DPICO_PLATFORM=rp2350 -DUSB_HID_ENABLED=1 -DDEBUG_LOGS_ENABLED=ON ..
make -j4
