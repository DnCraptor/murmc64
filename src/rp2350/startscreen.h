/*
 * startscreen.h - Welcome/start screen for murmfrodo4
 *
 * Displays system information on startup before emulation begins.
 */

#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *title;
    const char *subtitle;
    const char *version;
    uint32_t cpu_mhz;
    uint32_t psram_mhz;
    uint8_t board_variant;
} startscreen_info_t;

/**
 * Display the start screen with system information
 * Blocks for a few seconds before returning
 *
 * @param info System information to display
 * @return 0 on success
 */
int startscreen_show(startscreen_info_t *info);

#ifdef __cplusplus
}
#endif

#endif // STARTSCREEN_H
