# murmfrodo4

C64 Emulator for RP2350 based on Frodo4.

## Features

- C64 emulation on RP2350 microcontroller
- HDMI video output (320x240)
- PS/2 keyboard support
- NES/SNES gamepad support
- D64 disk image loading from SD card
- Keyboard joystick emulation

## Keyboard Mapping

The keyboard layout uses VICE-style positional mapping. The main keyboard rows map directly to C64 positions:

```
PC:  ` 1 2 3 4 5 6 7 8 9 0 - =
C64: <- 1 2 3 4 5 6 7 8 9 0 + -

PC:  Q W E R T Y U I O P [ ]
C64: Q W E R T Y U I O P @ *

PC:  A S D F G H J K L ; '
C64: A S D F G H J K L : ;

PC:  Z X C V B N M , . /
C64: Z X C V B N M , . /
```

### Special Keys

| PC Key      | C64 Key        |
|-------------|----------------|
| Esc         | RUN/STOP       |
| Backspace   | INS/DEL        |
| Return      | RETURN         |
| Shift       | SHIFT          |
| Caps Lock   | SHIFT LOCK     |
| Tab         | CTRL           |
| L-Ctrl      | CTRL           |
| L-Alt       | C= (Commodore) |
| \           | ^ (up arrow)   |
| Home        | CLR/HOME       |
| End         | £ (pound)      |
| Page Up     | ^ (up arrow)   |
| Page Down   | = (equals)     |
| Insert      | Shift+INS/DEL  |
| Delete      | INS/DEL        |

### Function Keys and System Hotkeys

| PC Key        | Function           |
|---------------|--------------------|
| F1-F8         | C64 F1-F8          |
| F9            | Swap joystick port |
| F10           | Disk UI            |
| F11           | RESTORE (NMI)      |
| Ctrl+Alt+Del  | Reset C64          |

### Joystick Emulation

Arrow keys and modifier keys can be used for joystick control (active on port 2 by default):

| Key           | Joystick Action |
|---------------|-----------------|
| Arrow Up      | Up              |
| Arrow Down    | Down            |
| Arrow Left    | Left            |
| Arrow Right   | Right           |
| R-Ctrl        | Fire            |
| R-Alt         | Fire            |

Press **F9** to swap between joystick port 1 and port 2. Most games use port 2, but some use port 1.

Note: When using arrow keys for joystick, they don't send cursor keys to the C64.

## Disk Loading

Press **F10** to open the disk selector UI. Use arrow keys to navigate and Enter to select. ESC closes the menu.

## Building

```bash
mkdir build
cd build
cmake ..
make -j4
```

## License

Based on Frodo4 by Christian Bauer.
