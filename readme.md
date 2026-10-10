# Quake for ESP32-S3

A port of [MG24Quake](https://github.com/next-hack/MG24Quake), Nicola Wrachien's Quake port for
Silicon Labs EFR32MG24 microcontrollers (276 kB RAM), to the ESP32-S3. It runs the **full retail
Quake** on the [Seeed Wio Tracker L2 / L2 Pro](https://wiki.seeedstudio.com/meshtastic_wio_tracker_l2_intro/)
with its touch screen, speaker and microSD card, and on the
[LilyGO T-Deck](https://github.com/Xinyuan-LilyGO/T-Deck) with its keyboard and trackball, and on the
[Elecrow ThinkNode M9](https://www.elecrow.com/wiki/ThinkNode_M9_Meshtastic_Communication_Terminal_with_Full_Keyboard.html)
with its keypad, and on a [Seeed XIAO ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
with a [2.8" ILI9341 SPI module](https://done.land/components/humaninterface/display/tft/ili9341/2.8inch320x240/)
(MSP2806/MSP2807, with microSD slot). Bluetooth LE keyboards, mice and gamepads are supported on all of them. Other ESP32-S3 boards with PSRAM, a 320x240 SPI display and an SD card can be
configured in [`board_config.h`](QuakeESP32S3/main/board_config.h).

Based on WinQuake/SDLQuake by id Software, MG24 port by Nicola Wrachien (next-hack). The
technical background of the engine is described in the next-hack articles:

- [Quake port to Sparkfun and Arduino Nano Matter boards using only 276 kB RAM](https://next-hack.com/index.php/2024/09/22/quake-port-to-sparkfun-and-arduino-nano-matter-boards-using-only-276-kb-ram/)
- [Now even the full retail Quake runs on the Arduino Nano Matter board](https://next-hack.com/index.php/2024/11/02/now-even-the-full-retail-quake-version-runs-on-the-arduino-nano-matter-board-and-it-is-faster-as-well/)

## Features

- Full Quake engine (MG24Quake 2.0):
  - static and dynamic lighting with bilinear interpolation, Gouraud shading on enemies;
  - animated sky, turbulent surfaces (water, teleports…), underwater warp effect;
  - particles for smoke, blood, etc.
- 320x200 rendering (Quake's default resolution), stretched to the full 320x240 screen (the
  4:3 aspect ratio of the original monitors).
- Full game logic: monster AI, teleports, triggers, doors…
- Shareware and **full retail** episodes, including the skill/episode selection map.
- Sound: 8 bit stereo, 11025 Hz; static, ambient and dynamic sounds.
- Console with cheats, options with key remapping, savegames with full game state.
- Touch screen controls with multi-touch; Bluetooth LE keyboards, mice and gamepads.
- About 41-42 fps in the timedemos on the Wio Tracker L2 (ESP32-S3 at 240 MHz, both cores); frame
  rate counter in Options → Show FPS (or `showfps 1` in the console).

## Limitations

- No CD audio / music.
- No multiplayer.
- Fixed screen size: the status bar can't be reduced or removed.
- Bluetooth LE only (the ESP32-S3 has no Bluetooth Classic), one Bluetooth device at a time.
- The 2021 remaster `pak0.pak` is not supported: use the original (1.06) PAK files.

## Quick start (Wio Tracker L2)

You need the firmware image (`QuakeESP32S3-wio-tracker-l2.bin`, from the GitHub releases or
built as described below) and your own copy of Quake: the shareware `pak0.pak`, or the retail
`pak0.pak` + `pak1.pak`.

1. Convert the PAK files (see [PAK file](#pak-file)) and copy the result to the root of a
   FAT32-formatted microSD card as `PAK0.PAK`. Insert the card in the L2.
2. Flash the firmware. The L2 appears as a USB serial port; if flashing fails, hold BOOT while
   pressing RESET to enter download mode.

   ```sh
   esptool.py --chip esp32s3 -p <port> -b 921600 write_flash 0x0 QuakeESP32S3-wio-tracker-l2.bin
   ```

   This replaces the Meshtastic firmware. Back it up first if you want to go back
   (`esptool.py -p <port> read_flash 0 0x1000000 backup.bin`).
3. Press RESET. Boot messages appear on the screen and on the USB serial console.

## Quick start (T-Deck)

Same as for the L2, with the T-Deck build (`idf.py -B build-tdeck -DBOARD=BOARD_LILYGO_TDECK build`,
see [Building](#building)). To enter download mode, hold the trackball (BOOT) while switching
on or pressing RESET.

## Quick start (ThinkNode M9)

Same as for the L2, with the M9 build (`idf.py -B build-m9 -DBOARD=BOARD_THINKNODE_M9 build`).
The M9 USB port is a CH340 USB-serial bridge: flashing resets the board into download mode by
itself (use `-b 460800`: faster baud rates may fail), and the serial console is on it at 115200.

## Quick start (XIAO ESP32S3 + ILI9341 module)

Wire the module as below, then do as for the L2 with the XIAO build
(`idf.py -B build-xiao -DBOARD=BOARD_XIAO_ESP32S3 build`).

| Module pin | XIAO pin | GPIO |
|---|---|---|
| VCC | 3V3 | – |
| GND | GND | – |
| SCK + SD_SCK | D8 | 7 |
| SDI (MOSI) + SD_MOSI | D10 | 9 |
| SD_MISO | D9 | 8 |
| SDO (MISO) of the LCD | not connected | – |
| CS (LCD) | D0 | 1 |
| DC/RS | D1 | 2 |
| SD_CS | D2 | 3 |
| RESET | D3 | 4 |
| LED (backlight) | 3V3 | – |

There is no sound and the only button is BOOT (menu): play with a Bluetooth LE gamepad or
keyboard. GPIO buttons can be added on D4-D7 (GPIO 5, 6, 43, 44) with the `BUTTON_PIN_*` settings.
On the XIAO ESP32S3 Sense, the camera board's SD slot is on the same SPI lines: its CS (GPIO21)
is kept high, so a card in that slot does not interfere.

## Hardware

| Item | Wio Tracker L2 | T-Deck | ThinkNode M9 | XIAO ESP32S3 + module | Generic board |
|---|---|---|---|---|---|
| MCU | ESP32-S3, 16 MB flash, 8 MB octal PSRAM | ESP32-S3, 16 MB flash, 8 MB octal PSRAM | ESP32-S3, 16 MB flash, 8 MB octal PSRAM | ESP32-S3, 8 MB flash, 8 MB octal PSRAM | ESP32-S3 with PSRAM (8 MB recommended, e.g. N8R8 / N16R8), flash ≥ 4 MB |
| Display | 3.2" 320x240 NV3031B, QSPI | 2.8" 320x240 ST7789, SPI | 2.4" 320x240 ST7789, SPI | 2.8" 320x240 ILI9341, SPI | 320x240 SPI LCD, ST7789 or ILI9341 |
| Storage | microSD, SDMMC 1-bit | microSD, SPI (shared with the display) | microSD, SPI (shared with the display) | microSD, SPI (shared with the display) | SD card (FAT), SPI or SDMMC (1/4 bit) |
| Audio | ES8311 codec + speaker amplifier | MAX98357A I2S amplifier | passive buzzer, driven with PDM | none | optional I2S DAC/amplifier (MAX98357A, PCM5102…) or PDM on one pin |
| Input | GT911 touch screen, BOOT and WAKE buttons, Bluetooth LE | keyboard, trackball, Bluetooth LE | keypad, Bluetooth LE | BOOT button, Bluetooth LE | Bluetooth LE, optional GPIO buttons |

On the L2, power rails, display reset and the touch controller are driven through the PCA9555
GPIO expander, and the backlight through the LP5814 LED driver, all on the I2C bus (SDA 47,
SCL 48). See [`esp_board_l2.c`](QuakeESP32S3/main/esp_board_l2.c).

On the T-Deck, GPIO10 powers the peripherals, the keyboard is an ESP32-C3 at I2C address 0x55
(SDA 18, SCL 8) and the trackball pulses GPIO 3/15/1/2. See
[`esp_board_tdeck.c`](QuakeESP32S3/main/esp_board_tdeck.c).

On the ThinkNode M9, GPIO18 (active low) powers the peripherals and the keypad is an STC8H
microcontroller at I2C address 0x6C (V1 boards) or 0x6D (V2) on SDA 20, SCL 21. The console is on
UART0 (`sdkconfig.defaults.m9`, applied by the M9 build). See
[`esp_board_m9.c`](QuakeESP32S3/main/esp_board_m9.c).

Bluetooth: BLE keyboards and mice, Xbox Wireless controllers (firmware 5.x+), 8BitDo pads in
BLE/Android mode and generic BLE gamepads work. Bluetooth Classic-only devices (DualShock 4,
DualSense, most Switch Pro controllers, many older keyboards and mice) can't be used.

## Controls

### Touch screen

Once the screen has been touched, the buttons are drawn over the 3D view. They are hidden again
when a Bluetooth controller connects.

| Area | In game | In menus / console |
|---|---|---|
| Left half | virtual stick, centered where the thumb lands: move / strafe | swipe = arrow keys |
| Right half | drag to turn / look | swipe = arrow keys, tap = select |
| Big circle, bottom right | fire (drag from it to aim while firing) | |
| Middle circle, right | jump / swim up | |
| Small circle, top right | next weapon | |
| ☰ box, top left | menu | back |

Move and look at the same time with two thumbs. Layout, stick radius and look speed are at the
top of [`esp_touch.c`](QuakeESP32S3/main/esp_touch.c) and [`esp_input.c`](QuakeESP32S3/main/esp_input.c).

### Board buttons (L2)

BOOT opens the menu (escape); WAKE jumps in game and selects in menus.

### T-Deck keyboard and trackball

| Key | In game | In menus / console |
|---|---|---|
| W / S | forward / back | letters |
| A / D | strafe left / right | letters |
| Trackball | turn / look | arrow keys |
| Trackball click | fire | select |
| E | fire | letter |
| Q | next weapon | letter |
| Space | jump / swim up | space |
| Shift | run | shift |
| Sym + key | digits / symbols (1-8 select weapons) | digits / symbols |
| Alt | menu | back |
| Mic | console | console |
| Speaker (`$`) | scores | |
| Enter, Backspace | | enter, backspace |

Other letters keep their default Quake bindings in game. Held keys need the keyboard firmware
from June 2025 or later (raw matrix mode); with older keyboard firmware each key press is a short
tap. The serial log says which mode is in use.

### ThinkNode M9 keypad

| Key | In game | In menus / console |
|---|---|---|
| D-pad up / down | forward / back | arrow keys |
| D-pad left / right | turn | arrow keys |
| D-pad center (Enter), E | fire | select |
| W / S | forward / back | letters |
| A / D | strafe left / right | letters |
| Q, FM | next weapon | |
| Space | jump / swim up | space |
| Sym + key | digits / symbols (1-8 select weapons) | digits / symbols |
| Home | menu | back |
| Del | menu | back / backspace |
| Mute | scores | |
| Time | console | console |

The keypad reports single key presses only: it can't tell when a key is released, and keys
can't be combined. Each press is a short tap (movement keys last a bit longer); press repeatedly
to keep moving. Turning on *Always Run* in the options menu helps.

### Bluetooth LE keyboard, mouse and gamepad

A keyboard behaves like a PC keyboard (default Quake bindings, console with `` ` ``). Mouse
buttons are MOUSE1..3, the wheel is MWHEELUP/DOWN, and movement turns / looks.

Gamepad (standard "Android" button layout):

| Button | In game | In menus |
|---|---|---|
| Left stick | move / strafe | navigate |
| Right stick | turn / look | |
| D-pad | forward/back/turn (arrows) | navigate |
| RT | fire | select |
| LT | run | |
| A | jump / swim up | select |
| B | swim down | back |
| X | next weapon | |
| Y | scores | |
| LB / RB | strafe left / right | |
| Start / Home | menu | back |
| Select | console | console |

Button assignments are in the tables at the top of [`esp_input.c`](QuakeESP32S3/main/esp_input.c).

**Pairing:** put the device in pairing mode, and disconnect it from any phone or computer it is
paired with (or it will reconnect there). The ESP32-S3 keeps scanning for BLE HID devices, even
during the game, and connects to the strongest one (bonded devices are preferred), reconnecting
automatically. If a device is not found, set `BLE_HID_DEBUG` to 1 and check which advertisers
the serial log shows. To forget bonds, erase the NVS partition (`idf.py erase-flash` also
erases savegames).

## PAK file

The game data is read directly from the SD card, converted to the MG24Quake format (which
changed in MG24Quake 2.0: PAK files converted for older releases must be converted again).

1. Build the converter (or use the Windows executable in
   `QuakeESP32S3/tools/MCUPackConverter/bin/Debug/`):

   ```sh
   cc -O2 -o MCUPackConverter QuakeESP32S3/tools/MCUPackConverter/*.c -lm
   ```

2. Run it in a directory containing `pak0.pak` (and the retail `pak1.pak`, if you have it). It
   prints a lot of data and asks for a key press at the end. The result is a single
   `PAK0Conv.PAK`: `pak0` and `pak1` are joined.
3. Copy `PAK0Conv.PAK` to the root of the SD card as `PAK0.PAK`.

## Building

With ESP-IDF 5.5 installed and exported:

```sh
cd QuakeESP32S3
idf.py set-target esp32s3
idf.py build flash monitor                                        # Wio Tracker L2
idf.py -B build-tdeck -DBOARD=BOARD_LILYGO_TDECK build flash monitor   # T-Deck
idf.py -B build-m9 -DBOARD=BOARD_THINKNODE_M9 build flash monitor      # ThinkNode M9
idf.py -B build-xiao -DBOARD=BOARD_XIAO_ESP32S3 build flash monitor    # XIAO ESP32S3 + ILI9341 module
```

To create the merged image for a release:

```sh
cd QuakeESP32S3/build
esptool.py --chip esp32s3 merge_bin -o QuakeESP32S3-wio-tracker-l2.bin @flash_args
```

`platformio.ini` is provided for PlatformIO users (pioarduino platform, ESP-IDF 5.5) but has
not been tested.

### Configuration

Select the board with `BOARD` in [`board_config.h`](QuakeESP32S3/main/board_config.h)
(`BOARD_WIO_TRACKER_L2` by default, `BOARD_LILYGO_TDECK`, `BOARD_THINKNODE_M9`, `BOARD_XIAO_ESP32S3` or `BOARD_GENERIC`),
or pass `-DBOARD=...` to `idf.py` (each such build keeps its `sdkconfig` in its build directory). For a generic board set the display
SPI pins, controller type, `DISPLAY_MADCTL` (orientation) and color inversion, the SD card mode
and pins (the card can share the display SPI bus, at the cost of some speed), the audio output
and the optional GPIO buttons.

The default `sdkconfig.defaults` is for **octal PSRAM** modules (N8R8, N16R8). For quad PSRAM
modules (e.g. N8R2) replace `CONFIG_SPIRAM_MODE_OCT=y` with `CONFIG_SPIRAM_MODE_QUAD=y`.

| Option | Default | |
|---|---|---|
| `DISPLAY_STRETCH_TO_FULL_HEIGHT` | 1 | stretch 320x200 to 320x240; 0 = 1:1 pixels with black bars |
| `TOUCH_CONTROLS_ENABLED` | 1 | on-screen controls on boards with a touch controller |
| `TOUCH_SWAP_XY`, `TOUCH_INVERT_X/Y` | L2 values | touch to screen coordinate mapping |
| `TOUCH_DEBUG` | 0 | log raw touch coordinates |
| `BLE_HID_ENABLED` | 1 | Bluetooth LE HID host |
| `BLE_HID_DEBUG` | 0 | log every nearby BLE advertiser while scanning |

The T-Deck and M9 volume is `AUDIO_SAMPLE_SHIFT` (6 dB per step). The L2 speaker volume is set by ES8311 register `0x32` in `boardAudioCodecInit()`
([`esp_board_l2.c`](QuakeESP32S3/main/esp_board_l2.c), 0.5 dB steps, `0xBF` = 0 dB; the
default is -14 dB).

## Repository layout

| Path | Contents |
|---|---|
| `QuakeESP32S3/main` | ESP32-S3 platform code: display, SD/PAK access, audio, input, touch, Bluetooth, board support |
| `QuakeESP32S3/main/port` | headers replacing the MG24 platform headers used by the engine |
| `QuakeESP32S3/components/esp_hid` | ESP-IDF 5.5.5 `esp_hid` with an upstream fix: without it no BLE HID device works (see its README) |
| `QuakeESP32S3/quake` | the MG24Quake engine; ESP32 code paths are selected with `ESP_PLATFORM` |
| `QuakeESP32S3/tools/MCUPackConverter` | PAK converter |
| `QuakeESP32S3/tools/QuakeCConverter`, `FunctionArrayCreator`, `QTablesGenerator` | MG24Quake tools that generate engine sources (QuakeC progs, field accessors, lookup tables); only needed to change the game code |

## How it works

| MG24 | ESP32-S3 |
|---|---|
| PAK in 2 interleaved SPI flash chips, read with DMA | PAK read from the SD card in 8 kB blocks, cached in PSRAM (all free PSRAM, CLOCK replacement). Asynchronous reads complete immediately. |
| Savegames/settings at the end of the SPI flash | `quakenvm` flash partition, mapped at the same virtual address |
| Internal flash used as model/skin cache | 1 MB PSRAM buffer (`INTERNAL_FLASH_CACHE_SIZE`) |
| 20 kB auxiliary radio RAM (`AUX_SECTION`) | PSRAM |
| ARM inline assembly in the rasterizer | the portable C code paths (the ones used by the MG24 Windows build), except the lit wall spans (texture and light map sampled per pixel, as there is no surface cache): a C loop giving the same pixels as the assembly code, and the rows of each surface split between the two cores ([`esp_parallel.c`](QuakeESP32S3/main/esp_parallel.c)) |
| Display LDMA with progressive refresh | display task on core 0 converts the 8-bit frame through the palette (stretching it to 240 lines and drawing the touch overlay) and sends it by SPI/QSPI DMA, updating the same "lines sent" counter the renderer waits on |
| Audio DAC + LDMA ring buffer | audio task playing the same ring buffer through I2S |

The game runs on core 1 and never yields; display, audio, touch and Bluetooth run on core 0. While
the walls are drawn, a low priority worker task on core 0 draws the odd rows of each surface.

Flash and PSRAM run at 120 MHz (about 6% faster than 80 MHz). ESP-IDF marks octal PSRAM at
120 MHz as experimental: if the game crashes at random once warm, set `CONFIG_ESPTOOLPY_FLASHFREQ_80M`
and `CONFIG_SPIRAM_SPEED_80M` in `sdkconfig.defaults` and delete the build directory.

### Benchmark

`idf.py -B build-bench -DFORCE_TIME_DEMO=2 build flash monitor` (add `-DBOARD=...`) plays the
demo loop as timedemos and prints the result of each demo (`970 frames 23097 ms 41.995 fps`).

| Wio Tracker L2, timedemo | demo1 | demo2 | demo3 |
|---|---|---|---|
| before (80 MHz, one core) | 23.6 | 23.9 | 23.3 |
| 120 MHz flash and PSRAM | 25.1 | 25.4 | 24.7 |
| C lit span loop | 34.3 | 34.7 | 34.1 |
| spans on both cores, FPU reciprocal | 42.0 | 42.6 | 42.1 |

## License

GPL v2, see [LICENSE](LICENSE). The tiny printf implementation
([`printf.c`](QuakeESP32S3/main/printf.c), Marco Paland) is MIT licensed.

Quake game data (PAK files) is not included and is not covered by this license: you need your
own copy of the game (the shareware `pak0.pak` is freely distributable).
