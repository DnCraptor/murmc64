/*
 *  sysconfig.h - System configuration for Frodo4
 *
 *  Frodo4 C64 Emulator - RP2350 Port
 *
 *  This file defines platform-specific configuration.
 */

#ifndef SYSCONFIG_H
#define SYSCONFIG_H

//=============================================================================
// Platform Detection
//=============================================================================

#ifdef FRODO_RP2350
// RP2350 platform
#define FRODO_PLATFORM "RP2350"
#else
// Desktop platform (SDL)
#define FRODO_PLATFORM "Desktop"
#endif

//=============================================================================
// Feature Configuration
//=============================================================================

// Disable features not supported on RP2350
#ifdef FRODO_RP2350
// No GTK GUI
#undef HAVE_GTK
// No filesystem-based prefs
#define PREFS_MINIMAL 1
// No rewind buffer (too much memory)
#define NO_REWIND_BUFFER 1
// No cartridge ROM auto-loading
#define NO_AUTO_CARTRIDGE 1
#endif

//=============================================================================
// NTSC/PAL Configuration
//=============================================================================

// Default to PAL timing
#ifndef NTSC
// PAL mode (default)
#endif

//=============================================================================
// Optimization Hints
//=============================================================================

// Allow compiler to assume unaligned access is OK
// (RP2350 Cortex-M33 handles unaligned access)
#ifdef FRODO_RP2350
#define CAN_ACCESS_UNALIGNED
#endif

#endif // SYSCONFIG_H
