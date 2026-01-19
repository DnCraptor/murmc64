/*
 *  ROM_data.h - Standalone ROM data for RP2350
 *
 *  Frodo4 C64 Emulator - RP2350 Port
 *
 *  C64/1541 ROMs (C) Commodore Business Machines
 */

#ifndef ROM_DATA_H
#define ROM_DATA_H

#include <cstdint>
#include "../board_config.h"

// ROM declarations (defined in ROM_data.cpp)
// Uses sizes from board_config.h
extern const uint8_t BuiltinBasicROM[BASIC_ROM_SIZE];
extern const uint8_t BuiltinKernalROM[KERNAL_ROM_SIZE];
extern const uint8_t BuiltinCharROM[CHAR_ROM_SIZE];
extern const uint8_t BuiltinDriveROM[DRIVE_ROM_SIZE];

#endif // ROM_DATA_H
