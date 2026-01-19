/*
 *  sysdeps_rp2350.h - System-dependent definitions for RP2350
 *
 *  Frodo4 C64 Emulator - RP2350 Port
 *
 *  This file replaces SDL dependencies with RP2350-specific implementations.
 */

#ifndef SYSDEPS_RP2350_H
#define SYSDEPS_RP2350_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Platform Identification
//=============================================================================

#define FRODO_RP2350 1

//=============================================================================
// Time Functions (replacing SDL_GetTicks, etc.)
//=============================================================================

static inline uint32_t rp2350_get_ticks_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static inline uint64_t rp2350_get_ticks_us(void) {
    return to_us_since_boot(get_absolute_time());
}

static inline void rp2350_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

static inline void rp2350_delay_us(uint32_t us) {
    sleep_us(us);
}

//=============================================================================
// Memory Allocation (using PSRAM for large allocations)
//=============================================================================

// Forward declarations - implementations in psram_allocator.c
void *psram_malloc(size_t size);
void *psram_realloc(void *ptr, size_t size);
void psram_free(void *ptr);

// For C64 RAM, we want it in PSRAM
#define C64_MALLOC(size)        psram_malloc(size)
#define C64_FREE(ptr)           psram_free(ptr)
#define C64_REALLOC(ptr, size)  psram_realloc(ptr, size)

//=============================================================================
// Atomic Operations
//=============================================================================

// RP2350 has hardware spinlocks, but for simple flags we can use volatile
#define ATOMIC_LOAD(ptr)        (*(volatile typeof(*(ptr)) *)(ptr))
#define ATOMIC_STORE(ptr, val)  (*(volatile typeof(*(ptr)) *)(ptr) = (val))

//=============================================================================
// Debug Output
//=============================================================================

#if ENABLE_DEBUG_LOGS
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif // SYSDEPS_RP2350_H
