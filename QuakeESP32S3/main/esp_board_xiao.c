/**
 * @file esp_board_xiao.c
 * @brief Seeed XIAO ESP32S3 + ILI9341 module bring-up: keeps the SD card chip
 * selects high until their drivers take over the pins.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include "esp_board.h"
#include "board_config.h"

#if BOARD == BOARD_XIAO_ESP32S3
#include "driver/gpio.h"

void boardInit(void)
{
    // the SD cards (module, and the Sense camera board if present) must not
    // answer while the display is initialized on the shared bus
    gpio_config_t io = { 0 };
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = (1ULL << SD_PIN_CS) | (1ULL << BOARD_PIN_SENSE_SD_CS);
    gpio_config(&io);
    gpio_set_level(SD_PIN_CS, 1);
    gpio_set_level(BOARD_PIN_SENSE_SD_CS, 1);
}

void boardBacklightOn(void)
{
}

void boardAudioCodecInit(void)
{
}

bool boardWakeButtonPressed(void)
{
    return false;
}

bool boardHasTouch(void)
{
    return false;
}

int boardTouchRead(boardTouchPoint_t *points)
{
    (void) points;
    return -1;
}
#endif
