/*
 *  input_rp2350.cpp - Input handling for RP2350
 *
 *  Frodo4 C64 Emulator - RP2350 Port
 *
 *  Handles PS/2 keyboard, USB HID keyboard, and NES/SNES gamepad input.
 */

#include "board_config.h"

extern "C" {
#include "debug_log.h"
#include "pico/stdlib.h"
#include "nespad/nespad.h"

#if ENABLE_PS2_KEYBOARD
#include "ps2kbd/ps2kbd_wrapper.h"
#endif

#ifdef USB_HID_ENABLED
#include "usbhid/usbhid_wrapper.h"
#endif
}

#include <cstring>

//=============================================================================
// C64 Keyboard Matrix Mapping
//=============================================================================

/*
  C64 keyboard matrix:

    Bit 7   6   5   4   3   2   1   0
  0    CUD  F5  F3  F1  F7 CLR RET DEL
  1    SHL  E   S   Z   4   A   W   3
  2     X   T   F   C   6   D   R   5
  3     V   U   H   B   8   G   Y   7
  4     N   O   K   M   0   J   I   9
  5     ,   @   :   .   -   L   P   +
  6     /   ↑   =  SHR HOM  ;   *   £
  7    R/S  Q   C= SPC  2  CTL  ←   1
*/

#define MATRIX(a, b) (((a) << 3) | (b))

//=============================================================================
// PS/2 Scancode to C64 Matrix Translation
//=============================================================================

// PS/2 Set 2 scancodes -> C64 matrix position
// Format: PS/2 scancode (with 0xF0 prefix for break) -> C64 matrix byte:bit
static const int8_t ps2_to_c64[128] = {
    -1,     // 0x00
    -1,     // 0x01 F9
    -1,     // 0x02
    -1,     // 0x03 F5 (handled separately)
    -1,     // 0x04 F3 (handled separately)
    -1,     // 0x05 F1 (handled separately)
    -1,     // 0x06 F2
    -1,     // 0x07 F12
    -1,     // 0x08
    -1,     // 0x09 F10
    -1,     // 0x0A F8
    -1,     // 0x0B F6
    -1,     // 0x0C F4
    MATRIX(7, 2),  // 0x0D Tab -> CTRL
    MATRIX(7, 1),  // 0x0E ` -> ←
    -1,     // 0x0F

    -1,     // 0x10
    MATRIX(7, 5),  // 0x11 L-Alt -> C=
    MATRIX(1, 7),  // 0x12 L-Shift
    -1,     // 0x13
    MATRIX(7, 2),  // 0x14 L-Ctrl
    MATRIX(7, 6),  // 0x15 Q
    MATRIX(7, 0),  // 0x16 1
    -1,     // 0x17
    -1,     // 0x18
    -1,     // 0x19
    MATRIX(1, 4),  // 0x1A Z
    MATRIX(1, 5),  // 0x1B S
    MATRIX(1, 2),  // 0x1C A
    MATRIX(1, 1),  // 0x1D W
    MATRIX(7, 3),  // 0x1E 2
    -1,     // 0x1F

    -1,     // 0x20
    MATRIX(2, 4),  // 0x21 C
    MATRIX(2, 7),  // 0x22 X
    MATRIX(2, 2),  // 0x23 D
    MATRIX(1, 6),  // 0x24 E
    MATRIX(1, 3),  // 0x25 4
    MATRIX(1, 0),  // 0x26 3
    -1,     // 0x27
    -1,     // 0x28
    MATRIX(7, 4),  // 0x29 Space
    MATRIX(3, 7),  // 0x2A V
    MATRIX(2, 5),  // 0x2B F
    MATRIX(2, 6),  // 0x2C T
    MATRIX(2, 1),  // 0x2D R
    MATRIX(2, 0),  // 0x2E 5
    -1,     // 0x2F

    -1,     // 0x30
    MATRIX(4, 7),  // 0x31 N
    MATRIX(3, 4),  // 0x32 B
    MATRIX(3, 5),  // 0x33 H
    MATRIX(3, 2),  // 0x34 G
    MATRIX(3, 1),  // 0x35 Y
    MATRIX(2, 3),  // 0x36 6
    -1,     // 0x37
    -1,     // 0x38
    -1,     // 0x39
    MATRIX(4, 4),  // 0x3A M
    MATRIX(4, 2),  // 0x3B J
    MATRIX(3, 6),  // 0x3C U
    MATRIX(3, 0),  // 0x3D 7
    MATRIX(3, 3),  // 0x3E 8
    -1,     // 0x3F

    -1,     // 0x40
    MATRIX(5, 7),  // 0x41 ,
    MATRIX(4, 5),  // 0x42 K
    MATRIX(4, 1),  // 0x43 I
    MATRIX(4, 6),  // 0x44 O
    MATRIX(4, 3),  // 0x45 0
    MATRIX(4, 0),  // 0x46 9
    -1,     // 0x47
    -1,     // 0x48
    MATRIX(5, 4),  // 0x49 .
    MATRIX(6, 7),  // 0x4A /
    MATRIX(5, 2),  // 0x4B L
    MATRIX(5, 5),  // 0x4C ;/:
    MATRIX(5, 1),  // 0x4D P
    MATRIX(5, 3),  // 0x4E -
    -1,     // 0x4F

    -1,     // 0x50
    -1,     // 0x51
    MATRIX(6, 2),  // 0x52 '
    -1,     // 0x53
    MATRIX(5, 6),  // 0x54 [/@
    MATRIX(5, 0),  // 0x55 =/+
    -1,     // 0x56
    -1,     // 0x57
    -1,     // 0x58 Caps Lock
    MATRIX(6, 4),  // 0x59 R-Shift
    MATRIX(0, 1),  // 0x5A Enter
    MATRIX(6, 1),  // 0x5B ]/*
    -1,     // 0x5C
    MATRIX(6, 0),  // 0x5D \ -> £
    -1,     // 0x5E
    -1,     // 0x5F

    -1,     // 0x60
    -1,     // 0x61
    -1,     // 0x62
    -1,     // 0x63
    -1,     // 0x64
    -1,     // 0x65
    MATRIX(0, 0),  // 0x66 Backspace -> DEL
    -1,     // 0x67
    -1,     // 0x68
    -1,     // 0x69 KP 1
    -1,     // 0x6A
    -1,     // 0x6B KP 4
    -1,     // 0x6C KP 7
    -1,     // 0x6D
    -1,     // 0x6E
    -1,     // 0x6F

    -1,     // 0x70 KP 0
    -1,     // 0x71 KP .
    -1,     // 0x72 KP 2
    -1,     // 0x73 KP 5
    -1,     // 0x74 KP 6
    -1,     // 0x75 KP 8
    MATRIX(7, 7),  // 0x76 Escape -> RUN/STOP
    -1,     // 0x77 Num Lock
    -1,     // 0x78 F11
    -1,     // 0x79 KP +
    -1,     // 0x7A KP 3
    -1,     // 0x7B KP -
    -1,     // 0x7C KP *
    -1,     // 0x7D KP 9
    -1,     // 0x7E Scroll Lock
    -1,     // 0x7F
};

// Extended PS/2 scancodes (0xE0 prefix)
// Lookup function instead of designated initializer (not supported in C++)
static inline int8_t get_extended_c64_key(uint8_t scancode) {
    switch (scancode) {
        case 0x11: return MATRIX(7, 5);  // R-Alt -> C=
        case 0x14: return MATRIX(7, 2);  // R-Ctrl
        case 0x6B: return MATRIX(0, 2);  // Left arrow (with shift for cursor left)
        case 0x72: return MATRIX(0, 7);  // Down arrow
        case 0x74: return MATRIX(0, 2);  // Right arrow
        case 0x75: return MATRIX(0, 7);  // Up arrow (with shift)
        case 0x6C: return MATRIX(6, 3);  // Home -> CLR/HOME
        case 0x69: return MATRIX(6, 0);  // End -> £
        case 0x7D: return MATRIX(6, 6);  // Page Up -> ↑
        case 0x7A: return MATRIX(6, 5);  // Page Down -> =
        case 0x71: return MATRIX(0, 0);  // Delete -> INS/DEL
        default: return -1;
    }
}

// Map ASCII characters to C64 matrix position
// Returns -1 if not mappable, or MATRIX(row, col) | 0x100 if shift needed
static int ascii_to_c64_matrix(unsigned char key) {
    switch (key) {
        // Letters (uppercase)
        case 'A': return MATRIX(1, 2);
        case 'B': return MATRIX(3, 4);
        case 'C': return MATRIX(2, 4);
        case 'D': return MATRIX(2, 2);
        case 'E': return MATRIX(1, 6);
        case 'F': return MATRIX(2, 5);
        case 'G': return MATRIX(3, 2);
        case 'H': return MATRIX(3, 5);
        case 'I': return MATRIX(4, 1);
        case 'J': return MATRIX(4, 2);
        case 'K': return MATRIX(4, 5);
        case 'L': return MATRIX(5, 2);
        case 'M': return MATRIX(4, 4);
        case 'N': return MATRIX(4, 7);
        case 'O': return MATRIX(4, 6);
        case 'P': return MATRIX(5, 1);
        case 'Q': return MATRIX(7, 6);
        case 'R': return MATRIX(2, 1);
        case 'S': return MATRIX(1, 5);
        case 'T': return MATRIX(2, 6);
        case 'U': return MATRIX(3, 6);
        case 'V': return MATRIX(3, 7);
        case 'W': return MATRIX(1, 1);
        case 'X': return MATRIX(2, 7);
        case 'Y': return MATRIX(3, 1);
        case 'Z': return MATRIX(1, 4);

        // Numbers
        case '1': return MATRIX(7, 0);
        case '2': return MATRIX(7, 3);
        case '3': return MATRIX(1, 0);
        case '4': return MATRIX(1, 3);
        case '5': return MATRIX(2, 0);
        case '6': return MATRIX(2, 3);
        case '7': return MATRIX(3, 0);
        case '8': return MATRIX(3, 3);
        case '9': return MATRIX(4, 0);
        case '0': return MATRIX(4, 3);

        // Punctuation
        case ' ': return MATRIX(7, 4);
        case ',': return MATRIX(5, 7);
        case '.': return MATRIX(5, 4);
        case '/': return MATRIX(6, 7);
        case ';': return MATRIX(6, 2);
        case ':': return MATRIX(5, 5);
        case '=': return MATRIX(6, 5);
        case '+': return MATRIX(5, 0);
        case '-': return MATRIX(5, 3);
        case '*': return MATRIX(6, 1);
        case '@': return MATRIX(5, 6);

        // Special keys
        case 0x0D: return MATRIX(0, 1);  // Enter/Return
        case 0x08: return MATRIX(0, 0);  // Backspace -> DEL
        case 0x1B: return MATRIX(7, 7);  // Escape -> RUN/STOP
        case 0x09: return MATRIX(7, 2);  // Tab -> CTRL

        // Arrow keys (from ps2kbd_wrapper)
        case 0x15: return MATRIX(0, 2);  // Right (CTRL+U)
        case 0x0A: return MATRIX(0, 7);  // Down (CTRL+J)
        case 0x0B: return MATRIX(0, 7) | 0x100;  // Up (needs shift)

        // Function keys
        case 0xF1: return MATRIX(0, 4);  // F1
        case 0xF2: return MATRIX(0, 4) | 0x100;  // F2 (F1+shift)
        case 0xF3: return MATRIX(0, 5);  // F3
        case 0xF4: return MATRIX(0, 5) | 0x100;  // F4 (F3+shift)
        case 0xF5: return MATRIX(0, 6);  // F5
        case 0xF6: return MATRIX(0, 6) | 0x100;  // F6 (F5+shift)
        case 0xF7: return MATRIX(0, 3);  // F7
        case 0xF8: return MATRIX(0, 3) | 0x100;  // F8 (F7+shift)

        default: return -1;
    }
}

//=============================================================================
// Input State
//=============================================================================

static struct {
    // C64 keyboard matrix state
    uint8_t key_matrix[8];
    uint8_t rev_matrix[8];

    // Joystick state
    uint8_t joystick1;
    uint8_t joystick2;

    // PS/2 state
    bool ps2_extended;
    bool ps2_release;

} input_state;

//=============================================================================
// Input Functions
//=============================================================================

extern "C" {

void input_rp2350_init(void)
{
    // Initialize key matrices (all keys released = all bits set)
    memset(input_state.key_matrix, 0xFF, sizeof(input_state.key_matrix));
    memset(input_state.rev_matrix, 0xFF, sizeof(input_state.rev_matrix));

    // Initialize joysticks (all directions/buttons released)
    input_state.joystick1 = 0x1F;
    input_state.joystick2 = 0x1F;

    // Initialize gamepad
    nespad_begin(CPU_CLOCK_MHZ * 1000, NESPAD_GPIO_CLK, NESPAD_GPIO_DATA, NESPAD_GPIO_LATCH);

#if ENABLE_PS2_KEYBOARD
    // Initialize PS/2 keyboard
    printf("Initializing PS/2 keyboard on CLK=%d DATA=%d\n", PS2_PIN_CLK, PS2_PIN_DATA);
    ps2kbd_init();
#endif

#ifdef USB_HID_ENABLED
    // Initialize USB HID
    usbhid_wrapper_init();
#endif

    printf("Input initialized\n");
}

static void process_ps2_scancode(uint8_t scancode)
{
    // Handle special prefixes
    if (scancode == 0xE0) {
        input_state.ps2_extended = true;
        return;
    }
    if (scancode == 0xF0) {
        input_state.ps2_release = true;
        return;
    }

    int c64_key = -1;
    bool add_shift = false;

    if (input_state.ps2_extended) {
        // Extended scancode
        if (scancode < 128) {
            c64_key = get_extended_c64_key(scancode);

            // Arrow keys need shift handling
            if (scancode == 0x6B || scancode == 0x75) {
                add_shift = true;  // Left and Up need shift
            }
        }
        input_state.ps2_extended = false;
    } else {
        // Regular scancode
        if (scancode < 128) {
            c64_key = ps2_to_c64[scancode];
        }

        // Handle function keys specially
        switch (scancode) {
            case 0x05: c64_key = MATRIX(0, 4); break;  // F1
            case 0x06: c64_key = MATRIX(0, 4); add_shift = true; break;  // F2
            case 0x04: c64_key = MATRIX(0, 5); break;  // F3
            case 0x0C: c64_key = MATRIX(0, 5); add_shift = true; break;  // F4
            case 0x03: c64_key = MATRIX(0, 6); break;  // F5
            case 0x0B: c64_key = MATRIX(0, 6); add_shift = true; break;  // F6
            case 0x83: c64_key = MATRIX(0, 3); break;  // F7
            case 0x0A: c64_key = MATRIX(0, 3); add_shift = true; break;  // F8
        }
    }

    if (c64_key < 0) {
        input_state.ps2_release = false;
        return;
    }

    int c64_byte = (c64_key >> 3) & 7;
    int c64_bit = c64_key & 7;

    if (input_state.ps2_release) {
        // Key released
        if (add_shift) {
            input_state.key_matrix[6] |= 0x10;  // Right shift
            input_state.rev_matrix[4] |= 0x40;
        }
        input_state.key_matrix[c64_byte] |= (1 << c64_bit);
        input_state.rev_matrix[c64_bit] |= (1 << c64_byte);
        input_state.ps2_release = false;
    } else {
        // Key pressed
        if (add_shift) {
            input_state.key_matrix[6] &= ~0x10;  // Right shift
            input_state.rev_matrix[4] &= ~0x40;
        }
        input_state.key_matrix[c64_byte] &= ~(1 << c64_bit);
        input_state.rev_matrix[c64_bit] &= ~(1 << c64_byte);
    }
}

// Helper to set/clear a key in the C64 matrix
static void set_c64_key(int c64_key, bool pressed) {
    if (c64_key < 0) return;

    bool need_shift = (c64_key & 0x100) != 0;
    c64_key &= 0xFF;

    int c64_byte = (c64_key >> 3) & 7;
    int c64_bit = c64_key & 7;

    if (pressed) {
        // Key pressed
        if (need_shift) {
            input_state.key_matrix[6] &= ~0x10;  // Right shift
            input_state.rev_matrix[4] &= ~0x40;
        }
        input_state.key_matrix[c64_byte] &= ~(1 << c64_bit);
        input_state.rev_matrix[c64_bit] &= ~(1 << c64_byte);
    } else {
        // Key released
        if (need_shift) {
            input_state.key_matrix[6] |= 0x10;  // Right shift
            input_state.rev_matrix[4] |= 0x40;
        }
        input_state.key_matrix[c64_byte] |= (1 << c64_bit);
        input_state.rev_matrix[c64_bit] |= (1 << c64_byte);
    }
}

void input_rp2350_poll(uint8_t *key_matrix, uint8_t *rev_matrix, uint8_t *joystick)
{
#if ENABLE_PS2_KEYBOARD
    // Poll PS/2 keyboard
    ps2kbd_tick();
    int pressed;
    unsigned char key;
    while (ps2kbd_get_key(&pressed, &key)) {
        printf("PS2: key=0x%02X %s\n", key, pressed ? "DOWN" : "UP");
        int c64_key = ascii_to_c64_matrix(key);
        if (c64_key >= 0) {
            set_c64_key(c64_key, pressed != 0);
        }
    }

    // Handle shift key from modifiers
    uint8_t mods = ps2kbd_get_modifiers();
    if (mods & 0x22) {  // L/R Shift
        input_state.key_matrix[1] &= ~0x80;  // Left shift
        input_state.rev_matrix[7] &= ~0x02;
    } else {
        input_state.key_matrix[1] |= 0x80;
        input_state.rev_matrix[7] |= 0x02;
    }
#endif

#ifdef USB_HID_ENABLED
    // Process USB HID
    usbhid_wrapper_poll();

    int pressed;
    unsigned char key;
    while (usbhid_wrapper_get_key(&pressed, &key)) {
        int c64_key = ascii_to_c64_matrix(key);
        if (c64_key >= 0) {
            set_c64_key(c64_key, pressed != 0);
        }
    }

    // Handle shift key from USB modifiers
    uint8_t mods = usbhid_wrapper_get_modifiers();
    if (mods & 0x22) {  // L/R Shift
        input_state.key_matrix[1] &= ~0x80;
        input_state.rev_matrix[7] &= ~0x02;
    } else {
        input_state.key_matrix[1] |= 0x80;
        input_state.rev_matrix[7] |= 0x02;
    }
#endif

    // Poll gamepad
    nespad_read();
    uint8_t pad = nespad_state;

    // Map NES pad to C64 joystick
    // NES: Right=0x01, Left=0x02, Down=0x04, Up=0x08, Start=0x10, Select=0x20, B=0x40, A=0x80
    // C64: Up=0x01, Down=0x02, Left=0x04, Right=0x08, Fire=0x10

    uint8_t joy = 0x1F;  // All released

    if (pad & 0x08) joy &= ~0x01;  // Up
    if (pad & 0x04) joy &= ~0x02;  // Down
    if (pad & 0x02) joy &= ~0x04;  // Left
    if (pad & 0x01) joy &= ~0x08;  // Right
    if (pad & 0xC0) joy &= ~0x10;  // A or B -> Fire

    input_state.joystick1 = joy;

    // Second joystick from second gamepad
    uint8_t pad2 = nespad_state2;
    uint8_t joy2 = 0x1F;

    if (pad2 & 0x08) joy2 &= ~0x01;
    if (pad2 & 0x04) joy2 &= ~0x02;
    if (pad2 & 0x02) joy2 &= ~0x04;
    if (pad2 & 0x01) joy2 &= ~0x08;
    if (pad2 & 0xC0) joy2 &= ~0x10;

    input_state.joystick2 = joy2;

    // Return state
    memcpy(key_matrix, input_state.key_matrix, 8);
    memcpy(rev_matrix, input_state.rev_matrix, 8);
    *joystick = input_state.joystick1;
}

// Get joystick 2 state (for port 1 emulation)
uint8_t input_get_joystick2(void)
{
    return input_state.joystick2;
}

}  // extern "C"
