/*
 *  disk_loader.c - SD card disk image loader for RP2350
 *
 *  Frodo4 C64 Emulator - RP2350 Port
 *
 *  Scans SD card for D64/G64/T64 disk images and provides
 *  a simple interface for mounting them.
 */

#include "board_config.h"
#include "debug_log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fatfs/ff.h"
#include "psram_allocator.h"

//=============================================================================
// Configuration
//=============================================================================

#define MAX_DISK_IMAGES     100
#define MAX_FILENAME_LEN    64
#define DISK_SCAN_PATH      "/c64"

//=============================================================================
// Disk Image Entry
//=============================================================================

typedef struct {
    char filename[MAX_FILENAME_LEN];
    uint32_t size;
    uint8_t type;  // 0=D64, 1=G64, 2=T64, 3=TAP, 4=PRG, 5=CRT
} disk_entry_t;

//=============================================================================
// State
//=============================================================================

static struct {
    disk_entry_t entries[MAX_DISK_IMAGES];
    int count;
    bool initialized;
} disk_loader;

//=============================================================================
// File Type Detection
//=============================================================================

static int detect_file_type(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if (!ext) return -1;

    if (strcasecmp(ext, ".d64") == 0) return 0;
    if (strcasecmp(ext, ".g64") == 0) return 1;
    if (strcasecmp(ext, ".t64") == 0) return 2;
    if (strcasecmp(ext, ".tap") == 0) return 3;
    if (strcasecmp(ext, ".prg") == 0) return 4;
    if (strcasecmp(ext, ".crt") == 0) return 5;

    return -1;
}

//=============================================================================
// Public API
//=============================================================================

void disk_loader_init(void)
{
    memset(&disk_loader, 0, sizeof(disk_loader));
    disk_loader.initialized = true;

    MII_DEBUG_PRINTF("Disk loader initialized\n");
}

void disk_loader_scan(void)
{
    if (!disk_loader.initialized) {
        disk_loader_init();
    }

    disk_loader.count = 0;

    DIR dir;
    FILINFO fno;

    // Try to open disk directory
    FRESULT fr = f_opendir(&dir, DISK_SCAN_PATH);
    if (fr != FR_OK) {
        // Try root directory
        fr = f_opendir(&dir, "/");
        if (fr != FR_OK) {
            MII_DEBUG_PRINTF("Failed to open directory for scanning\n");
            return;
        }
    }

    MII_DEBUG_PRINTF("Scanning for disk images...\n");

    while (disk_loader.count < MAX_DISK_IMAGES) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) {
            break;  // End of directory
        }

        // Skip directories
        if (fno.fattrib & AM_DIR) {
            continue;
        }

        // Check file type
        int type = detect_file_type(fno.fname);
        if (type < 0) {
            continue;
        }

        // Add to list
        disk_entry_t *entry = &disk_loader.entries[disk_loader.count];
        strncpy(entry->filename, fno.fname, MAX_FILENAME_LEN - 1);
        entry->filename[MAX_FILENAME_LEN - 1] = '\0';
        entry->size = fno.fsize;
        entry->type = type;

        MII_DEBUG_PRINTF("  Found: %s (%lu bytes)\n", entry->filename, (unsigned long)entry->size);

        disk_loader.count++;
    }

    f_closedir(&dir);

    MII_DEBUG_PRINTF("Found %d disk images\n", disk_loader.count);
}

int disk_loader_get_count(void)
{
    return disk_loader.count;
}

const char *disk_loader_get_filename(int index)
{
    if (index < 0 || index >= disk_loader.count) {
        return NULL;
    }
    return disk_loader.entries[index].filename;
}

// Returns full path to disk image (static buffer - not thread safe)
const char *disk_loader_get_path(int index)
{
    static char path_buffer[128];

    if (index < 0 || index >= disk_loader.count) {
        return NULL;
    }

    snprintf(path_buffer, sizeof(path_buffer), "%s/%s",
             DISK_SCAN_PATH, disk_loader.entries[index].filename);

    return path_buffer;
}

uint32_t disk_loader_get_size(int index)
{
    if (index < 0 || index >= disk_loader.count) {
        return 0;
    }
    return disk_loader.entries[index].size;
}

int disk_loader_get_type(int index)
{
    if (index < 0 || index >= disk_loader.count) {
        return -1;
    }
    return disk_loader.entries[index].type;
}

// Load disk image into buffer (returns allocated buffer, caller must free)
uint8_t *disk_loader_load(int index, uint32_t *out_size)
{
    if (index < 0 || index >= disk_loader.count) {
        return NULL;
    }

    disk_entry_t *entry = &disk_loader.entries[index];

    // Build full path
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", DISK_SCAN_PATH, entry->filename);

    FIL file;
    FRESULT fr = f_open(&file, path, FA_READ);
    if (fr != FR_OK) {
        // Try root path
        snprintf(path, sizeof(path), "/%s", entry->filename);
        fr = f_open(&file, path, FA_READ);
        if (fr != FR_OK) {
            MII_DEBUG_PRINTF("Failed to open: %s\n", entry->filename);
            return NULL;
        }
    }

    // Allocate buffer in PSRAM
    uint8_t *buffer = (uint8_t *)psram_malloc(entry->size);
    if (!buffer) {
        f_close(&file);
        MII_DEBUG_PRINTF("Failed to allocate %lu bytes\n", (unsigned long)entry->size);
        return NULL;
    }

    // Read file
    UINT bytes_read;
    fr = f_read(&file, buffer, entry->size, &bytes_read);
    f_close(&file);

    if (fr != FR_OK || bytes_read != entry->size) {
        psram_free(buffer);
        MII_DEBUG_PRINTF("Failed to read file\n");
        return NULL;
    }

    if (out_size) {
        *out_size = entry->size;
    }

    MII_DEBUG_PRINTF("Loaded: %s (%lu bytes)\n", entry->filename, (unsigned long)entry->size);

    return buffer;
}
