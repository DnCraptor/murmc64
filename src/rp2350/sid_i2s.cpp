/*
 *  sid_i2s.cpp - SID audio output via I2S for RP2350
 *
 *  MurmC64 - Commodore 64 Emulator for RP2350
 *  Copyright (c) 2024-2026 Mikhail Matveev <xtreme@rh1.tech>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Stub implementation that outputs silence or test tones.
 *  Full SID emulation requires integrating the Frodo4 SID core.
 */

#include "../board_config.h"

extern "C" {
#include "debug_log.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"

// pico_audio_i2s from pico-extras
#define none pico_audio_enum_none
#include "pico/audio_i2s.h"
#undef none
}

#include <cstring>
#include <cstdio>

//=============================================================================
// Configuration
//=============================================================================

// Audio buffer configuration - use board_config.h defaults if not defined
#ifndef SID_SAMPLE_RATE
#define SID_SAMPLE_RATE     44100
#endif
#ifndef SID_BUFFER_SAMPLES
#define SID_BUFFER_SAMPLES  256
#endif
#ifndef SID_BUFFER_COUNT
#define SID_BUFFER_COUNT    4
#endif

// I2S pin configuration (from CMake defines)
#ifndef PICO_AUDIO_I2S_DATA_PIN
#ifdef BOARD_M1
#define PICO_AUDIO_I2S_DATA_PIN 26
#else
#define PICO_AUDIO_I2S_DATA_PIN 9
#endif
#endif

#ifndef PICO_AUDIO_I2S_CLOCK_PIN_BASE
#ifdef BOARD_M1
#define PICO_AUDIO_I2S_CLOCK_PIN_BASE 27
#else
#define PICO_AUDIO_I2S_CLOCK_PIN_BASE 10
#endif
#endif

#ifndef PICO_AUDIO_I2S_PIO
#define PICO_AUDIO_I2S_PIO 0
#endif

#ifndef PICO_AUDIO_I2S_DMA_CHANNEL
#define PICO_AUDIO_I2S_DMA_CHANNEL 6
#endif

#ifndef PICO_AUDIO_I2S_STATE_MACHINE
#define PICO_AUDIO_I2S_STATE_MACHINE 2
#endif

//=============================================================================
// I2S Audio State
//=============================================================================

// Ring buffer for SID samples
#define SID_RING_BUFFER_SIZE 2048

static struct {
    bool initialized;

    // Audio buffer pool
    struct audio_buffer_pool *producer_pool;

    // Ring buffer for samples from SID emulation
    int16_t ring_buffer[SID_RING_BUFFER_SIZE * 2];  // Stereo
    volatile uint32_t write_index;
    volatile uint32_t read_index;

    // Test tone state (for debugging)
    uint32_t phase;
    uint32_t phase_inc;

} audio_state;

// Audio format configuration (C++ compatible initialization)
static struct audio_format audio_format_config;
static struct audio_buffer_format producer_format_config;

static void init_audio_formats(void) {
    audio_format_config.format = AUDIO_BUFFER_FORMAT_PCM_S16;
    audio_format_config.sample_freq = SID_SAMPLE_RATE;
    audio_format_config.channel_count = 2;

    producer_format_config.format = &audio_format_config;
    producer_format_config.sample_stride = 4;  // 2 channels * 2 bytes per sample
}

//=============================================================================
// I2S Audio Functions
//=============================================================================

extern "C" {

void sid_i2s_init(void)
{
    if (audio_state.initialized) {
        return;
    }

    memset(&audio_state, 0, sizeof(audio_state));

    // Initialize audio format structs
    init_audio_formats();

    // Set test tone frequency (440 Hz A4)
    audio_state.phase_inc = (uint32_t)((440.0f / SID_SAMPLE_RATE) * 65536.0f);

    // Create audio buffer pool
    audio_state.producer_pool = audio_new_producer_pool(
        &producer_format_config,
        SID_BUFFER_COUNT,
        SID_BUFFER_SAMPLES
    );

    if (!audio_state.producer_pool) {
        MII_DEBUG_PRINTF("sid_i2s_init: failed to create producer pool\n");
        return;
    }

    // Configure I2S pins (C++ compatible initialization)
    struct audio_i2s_config config;
    memset(&config, 0, sizeof(config));
    config.data_pin = PICO_AUDIO_I2S_DATA_PIN;
    config.clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE;
    config.dma_channel = PICO_AUDIO_I2S_DMA_CHANNEL;
    config.pio_sm = PICO_AUDIO_I2S_STATE_MACHINE;

    // Setup I2S audio
    const struct audio_format *output_format = audio_i2s_setup(&audio_format_config, &config);
    if (!output_format) {
        MII_DEBUG_PRINTF("sid_i2s_init: audio_i2s_setup failed\n");
        return;
    }

    // Increase GPIO drive strength for I2S signals
    gpio_set_drive_strength(PICO_AUDIO_I2S_DATA_PIN, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PICO_AUDIO_I2S_CLOCK_PIN_BASE, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(PICO_AUDIO_I2S_CLOCK_PIN_BASE + 1, GPIO_DRIVE_STRENGTH_12MA);

    // Connect producer pool to I2S
    bool ok = audio_i2s_connect_extra(audio_state.producer_pool, false, 0, 0, NULL);
    if (!ok) {
        MII_DEBUG_PRINTF("sid_i2s_init: audio_i2s_connect_extra failed\n");
        return;
    }

    // Enable I2S
    audio_i2s_set_enabled(true);

    audio_state.initialized = true;
    MII_DEBUG_PRINTF("SID I2S audio initialized (stub - silence/test tone)\n");
}

void sid_i2s_update(void)
{
    if (!audio_state.initialized) {
        return;
    }

    // Transfer samples from ring buffer to I2S
    audio_buffer_t *buffer;

    while ((buffer = take_audio_buffer(audio_state.producer_pool, false)) != NULL) {
        int16_t *samples = (int16_t *)buffer->buffer->bytes;
        int sample_count = buffer->max_sample_count;

        for (int i = 0; i < sample_count; i++) {
            int16_t left = 0;
            int16_t right = 0;

            // Read from ring buffer if samples available
            uint32_t read_idx = audio_state.read_index;
            uint32_t write_idx = audio_state.write_index;

            if (read_idx != write_idx) {
                // Samples available
                uint32_t buf_idx = (read_idx % SID_RING_BUFFER_SIZE) * 2;
                left = audio_state.ring_buffer[buf_idx];
                right = audio_state.ring_buffer[buf_idx + 1];
                audio_state.read_index = read_idx + 1;
            }
            // else: buffer underrun, output silence

            // Stereo output
            samples[i * 2] = left;
            samples[i * 2 + 1] = right;
        }

        buffer->sample_count = sample_count;
        give_audio_buffer(audio_state.producer_pool, buffer);
    }
}

// Add samples to the SID ring buffer (called from SID emulation)
void sid_add_sample(int16_t left, int16_t right)
{
    if (!audio_state.initialized) {
        return;
    }

    uint32_t write_idx = audio_state.write_index;
    uint32_t read_idx = audio_state.read_index;

    // Check for buffer full (leave one slot empty)
    if (((write_idx + 1) % SID_RING_BUFFER_SIZE) != (read_idx % SID_RING_BUFFER_SIZE)) {
        uint32_t buf_idx = (write_idx % SID_RING_BUFFER_SIZE) * 2;
        audio_state.ring_buffer[buf_idx] = left;
        audio_state.ring_buffer[buf_idx + 1] = right;
        audio_state.write_index = write_idx + 1;
    }
    // else: buffer full, drop sample
}

// Get current sample buffer fill level
int sid_get_buffer_fill(void)
{
    int32_t fill = (int32_t)(audio_state.write_index - audio_state.read_index);
    if (fill < 0) fill += SID_RING_BUFFER_SIZE;
    return (int)fill;
}

}  // extern "C"
