/*
 *  disk_ui.c - On-screen disk selection UI for RP2350
 *
 *  Frodo4 C64 Emulator - RP2350 Port
 *
 *  Provides a simple menu for selecting disk images from SD card.
 */

#include "board_config.h"
#include "debug_log.h"

#include <stdio.h>
#include <string.h>

#include "HDMI.h"

//=============================================================================
// Forward Declarations
//=============================================================================

extern int disk_loader_get_count(void);
extern const char *disk_loader_get_filename(int index);

//=============================================================================
// UI State
//=============================================================================

static struct {
    bool visible;
    int selected_index;
    int scroll_offset;
    int items_per_page;
} disk_ui;

//=============================================================================
// UI Drawing (simplified - uses direct framebuffer access)
//=============================================================================

// Simple 8x8 font for the disk menu
static const uint8_t disk_ui_font[96][8] = {
    // Space (0x20) through ~ (0x7E)
    // Simplified - just define some basic characters
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // Space
    // ... (would define full font here)
};

static void draw_char(uint8_t *fb, int x, int y, char c, uint8_t color)
{
    if (c < 0x20 || c > 0x7E) c = ' ';

    // Simple character rendering
    // In a full implementation, we'd use a proper font
    // For now, just draw a simple block for non-space characters
    if (c != ' ') {
        for (int dy = 0; dy < 8; dy++) {
            for (int dx = 0; dx < 8; dx++) {
                int px = x + dx;
                int py = y + dy;
                if (px >= 0 && px < FB_WIDTH && py >= 0 && py < FB_HEIGHT) {
                    fb[py * FB_WIDTH + px] = color;
                }
            }
        }
    }
}

static void draw_string(uint8_t *fb, int x, int y, const char *str, uint8_t color)
{
    while (*str) {
        draw_char(fb, x, y, *str, color);
        x += 8;
        str++;
    }
}

static void draw_box(uint8_t *fb, int x, int y, int w, int h, uint8_t color)
{
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= 0 && px < FB_WIDTH && py >= 0 && py < FB_HEIGHT) {
                fb[py * FB_WIDTH + px] = color;
            }
        }
    }
}

//=============================================================================
// Public API
//=============================================================================

void disk_ui_init(void)
{
    disk_ui.visible = false;
    disk_ui.selected_index = 0;
    disk_ui.scroll_offset = 0;
    disk_ui.items_per_page = 20;  // Number of items visible at once
}

void disk_ui_show(void)
{
    disk_ui.visible = true;
    disk_ui.selected_index = 0;
    disk_ui.scroll_offset = 0;
}

void disk_ui_hide(void)
{
    disk_ui.visible = false;
}

bool disk_ui_is_visible(void)
{
    return disk_ui.visible;
}

void disk_ui_move_up(void)
{
    if (disk_ui.selected_index > 0) {
        disk_ui.selected_index--;
        if (disk_ui.selected_index < disk_ui.scroll_offset) {
            disk_ui.scroll_offset = disk_ui.selected_index;
        }
    }
}

void disk_ui_move_down(void)
{
    int count = disk_loader_get_count();
    if (disk_ui.selected_index < count - 1) {
        disk_ui.selected_index++;
        if (disk_ui.selected_index >= disk_ui.scroll_offset + disk_ui.items_per_page) {
            disk_ui.scroll_offset = disk_ui.selected_index - disk_ui.items_per_page + 1;
        }
    }
}

int disk_ui_get_selected(void)
{
    return disk_ui.selected_index;
}

void disk_ui_render(uint8_t *framebuffer)
{
    if (!disk_ui.visible || !framebuffer) {
        return;
    }

    int count = disk_loader_get_count();

    // Draw semi-transparent background
    draw_box(framebuffer, 20, 20, FB_WIDTH - 40, FB_HEIGHT - 40, 0);  // Black background

    // Draw title
    draw_string(framebuffer, 30, 25, "SELECT DISK IMAGE", 1);  // White

    // Draw horizontal line
    draw_box(framebuffer, 30, 35, FB_WIDTH - 60, 1, 12);  // Grey

    // Draw disk list
    int y = 40;
    for (int i = disk_ui.scroll_offset;
         i < count && i < disk_ui.scroll_offset + disk_ui.items_per_page;
         i++) {

        const char *filename = disk_loader_get_filename(i);
        if (!filename) continue;

        // Highlight selected item
        if (i == disk_ui.selected_index) {
            draw_box(framebuffer, 25, y - 1, FB_WIDTH - 50, 10, 6);  // Blue highlight
            draw_string(framebuffer, 30, y, filename, 1);  // White text
        } else {
            draw_string(framebuffer, 30, y, filename, 15);  // Light grey text
        }

        y += 10;
    }

    // Draw scroll indicators if needed
    if (disk_ui.scroll_offset > 0) {
        draw_string(framebuffer, FB_WIDTH - 40, 40, "^", 1);
    }
    if (disk_ui.scroll_offset + disk_ui.items_per_page < count) {
        draw_string(framebuffer, FB_WIDTH - 40, FB_HEIGHT - 35, "v", 1);
    }

    // Draw instructions
    draw_string(framebuffer, 30, FB_HEIGHT - 30, "UP/DOWN: SELECT  FIRE: LOAD  F11: CANCEL", 12);
}
