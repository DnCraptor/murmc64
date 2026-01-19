/*
 *  Display_rp2350.h - C64 graphics display for RP2350
 *
 *  Frodo4 C64 Emulator - RP2350 Port
 *
 *  Uses HDMI output via PIO/DMA.
 */

#ifndef DISPLAY_RP2350_H
#define DISPLAY_RP2350_H

#include "../Prefs.h"
#include <cstdint>
#include <string>

// Display dimensions (VIC output size)
// These may already be defined if we're included from Display.h
#ifndef FRODO_DISPLAY_DIMENSIONS_DEFINED
#define FRODO_DISPLAY_DIMENSIONS_DEFINED
constexpr unsigned DISPLAY_X = 0x180;  // 384 pixels
constexpr unsigned DISPLAY_Y = 0x110;  // 272 lines
constexpr unsigned NUM_NOTIFICATIONS = 3;
constexpr unsigned NOTIFICATION_LENGTH = 46;
#endif


class C64;


// Class for C64 graphics display on RP2350
class Display {
public:
    Display(C64 * c64);
    ~Display();

    void Pause();
    void Resume();

    void NewPrefs(const Prefs *prefs);

    void Update();

    void SetLEDs(int l0, int l1, int l2, int l3);
    void SetSpeedometer(int speed);
    void ShowNotification(std::string s);

    uint8_t * BitmapBase();
    int BitmapXMod();

    void PollKeyboard(uint8_t *key_matrix, uint8_t *rev_matrix, uint8_t *joystick);
    bool NumLock();

    // RP2350-specific: Get framebuffer for HDMI output
    uint8_t * GetFramebuffer() { return vic_pixels; }

private:
    void init_colors(int palette_prefs);
    void draw_overlays();
    void scale_to_hdmi();

    C64 * the_c64;                      // Pointer to C64 object

    int led_state[4];                   // Drive LED states

    uint8_t * vic_pixels;               // Buffer for VIC to draw into (DISPLAY_X * DISPLAY_Y)
    uint32_t palette[16];               // C64 color palette (ARGB)

    char speedometer_string[16];        // Speedometer text

    struct Notification {
        char text[NOTIFICATION_LENGTH]; // Notification text
        uint32_t time;                  // Time of notification (ms since boot)
        bool active;
    };

    Notification notes[NUM_NOTIFICATIONS];  // On-screen notifications
    unsigned next_note;                 // Index of next free notification

    bool num_locked;                    // For keyboard joystick swap
};


#endif // DISPLAY_RP2350_H
