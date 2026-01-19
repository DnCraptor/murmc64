/*
 *  sysdeps.h - Try to include the right system headers and get other
 *              system-specific stuff right
 *
 *  Frodo Copyright (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SYSDEPS_H
#define SYSDEPS_H

#include "sysconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <cstdint>
#include <string>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

//=============================================================================
// Platform-Specific Headers
//=============================================================================

#ifdef FRODO_RP2350

// RP2350 Platform
#include "pico/stdlib.h"
#include "pico/time.h"

// Time functions for RP2350
#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t GetTicks_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static inline uint64_t GetTicks_us(void) {
    return to_us_since_boot(get_absolute_time());
}

static inline void Delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

#ifdef __cplusplus
}
#endif

// Disable SDL-dependent features
#define NO_SDL 1

#else  // Desktop platform

// SDL Platform
#include <SDL.h>

// Time functions using SDL
static inline uint32_t GetTicks_ms(void) {
    return SDL_GetTicks();
}

static inline uint64_t GetTicks_us(void) {
    return SDL_GetTicks() * 1000ULL;
}

static inline void Delay_ms(uint32_t ms) {
    SDL_Delay(ms);
}

#endif  // FRODO_RP2350

//=============================================================================
// Memory Allocation
//=============================================================================

#ifdef FRODO_RP2350

// Use PSRAM allocator for large allocations on RP2350
#ifdef __cplusplus
extern "C" {
#endif

void *psram_malloc(size_t size);
void *psram_realloc(void *ptr, size_t size);
void psram_free(void *ptr);

#ifdef __cplusplus
}
#endif

// Macros for C64 memory allocation
#define C64_MALLOC(size)        psram_malloc(size)
#define C64_FREE(ptr)           psram_free(ptr)
#define C64_REALLOC(ptr, size)  psram_realloc(ptr, size)

#else  // Desktop

// Standard allocation on desktop
#define C64_MALLOC(size)        malloc(size)
#define C64_FREE(ptr)           free(ptr)
#define C64_REALLOC(ptr, size)  realloc(ptr, size)

#endif  // FRODO_RP2350

#endif // ndef SYSDEPS_H
