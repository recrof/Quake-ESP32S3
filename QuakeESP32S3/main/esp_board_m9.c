/**
 * @file esp_board_m9.c
 * @brief Elecrow ThinkNode M9 bring-up and input: peripheral power and the
 * keypad (STC8H controller at I2C address 0x6C on V1 boards, 0x6D on V2).
 *
 * Register 0x01 returns the code of the last key pressed (ASCII, or 0x8x/0xBx for
 * the function and navigation keys) once, then 0. Holding a key does not repeat
 * it and releases are not reported (the KB_INT line is only a short pulse, not
 * used here): every key press is a tap, the key is released after
 * KEYBOARD_TAP_MS (KEYBOARD_MOVE_TAP_MS for the movement keys in game).
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */
#include "esp_board.h"
#include "board_config.h"

#if BOARD == BOARD_THINKNODE_M9
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "quakedef.h"
#include "esp_input.h"

#define I2C_FREQ_HZ             100000
#define KEYBOARD_ADDR_V1        0x6C
#define KEYBOARD_ADDR_V2        0x6D
#define KEYBOARD_REG_KEY        0x01
#define KEYBOARD_POLL_MS        10
#define KEYBOARD_TAP_MS         150     // how long a key press lasts
#define KEYBOARD_MOVE_TAP_MS    300     // same, for the movement keys in game
#define KEYBOARD_DEBUG          0       // 1: log the key codes

// key codes of the function and navigation keys
#define M9_KEY_ENTER            0x0D
#define M9_KEY_DEL              0x08
#define M9_KEY_MUTE             0x81
#define M9_KEY_HOME             0x82
#define M9_KEY_TIME             0x83
#define M9_KEY_LIGHT            0x84
#define M9_KEY_FM               0x85
#define M9_KEY_FM_LONG          0x86
#define M9_KEY_PRESET           0x87
#define M9_KEY_INVALID          0x88
#define M9_KEY_DEL_LONG         0x89
#define M9_KEY_LEFT             0xB4
#define M9_KEY_UP               0xB5
#define M9_KEY_DOWN             0xB6
#define M9_KEY_RIGHT            0xB7

static const char *TAG = "m9";
static i2c_master_bus_handle_t i2cBus;
static i2c_master_dev_handle_t keyboard;

void boardInit(void)
{
    // peripherals power on, all the SPI chip selects high, GPS off
    gpio_config_t io = { 0 };
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = (1ULL << BOARD_PIN_POWER_ON) | (1ULL << BOARD_PIN_RADIO_CS) | (1ULL << SD_PIN_CS) |
        (1ULL << DISPLAY_PIN_CS) | (1ULL << BOARD_PIN_GPS_SLEEP) | (1ULL << KEYBOARD_PIN_LED);
    gpio_config(&io);
    gpio_set_level(BOARD_PIN_RADIO_CS, 1);
    gpio_set_level(SD_PIN_CS, 1);
    gpio_set_level(DISPLAY_PIN_CS, 1);
    gpio_set_level(BOARD_PIN_GPS_SLEEP, 1);
    gpio_set_level(KEYBOARD_PIN_LED, 0);
    gpio_set_level(BOARD_PIN_POWER_ON, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_master_bus_config_t busCfg =
    {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_I2C_SDA,
        .scl_io_num = BOARD_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&busCfg, &i2cBus));
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

// ---------------------------------------------------------------------------
// Keypad
// ---------------------------------------------------------------------------
// GPS enable: active low on V1 boards, active high on V2
static void gpsOff(int version)
{
    gpio_config_t io = { 0 };
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = 1ULL << BOARD_PIN_GPS_EN;
    gpio_config(&io);
    gpio_set_level(BOARD_PIN_GPS_EN, version > 1 ? 0 : 1);
}

static bool keyboardInit(void)
{
    uint8_t addr = 0;
    // the keypad controller boots after the power on: wait for it
    for (int i = 0; i < 20 && !addr; i++)
    {
        if (i2c_master_probe(i2cBus, KEYBOARD_ADDR_V2, 50) == ESP_OK)
            addr = KEYBOARD_ADDR_V2;
        else if (i2c_master_probe(i2cBus, KEYBOARD_ADDR_V1, 50) == ESP_OK)
            addr = KEYBOARD_ADDR_V1;
        else
            vTaskDelay(pdMS_TO_TICKS(100));
    }
    gpsOff(addr == KEYBOARD_ADDR_V2 ? 2 : 1);
    if (!addr)
    {
        ESP_LOGE(TAG, "Keypad not found");
        return false;
    }
    ESP_LOGI(TAG, "Keypad found at 0x%02X (M9 V%d)", addr, addr == KEYBOARD_ADDR_V2 ? 2 : 1);
    i2c_device_config_t devCfg =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    if (i2c_master_bus_add_device(i2cBus, &devCfg, &keyboard) != ESP_OK)
        return false;
    gpio_set_level(KEYBOARD_PIN_LED, 1);
    return true;
}

static int readKeyCode(void)
{
    uint8_t reg = KEYBOARD_REG_KEY, code;
    if (i2c_master_transmit_receive(keyboard, &reg, 1, &code, 1, 50) != ESP_OK)
        return -1;
    return code;
}

// Quake key for a keypad code (0: ignored)
static uint8_t gameKey(int code)
{
    bool inGame = key_dest == key_game;
    bool typing = key_dest == key_console || key_dest == key_message;
    switch (code)
    {
        case M9_KEY_UP: return K_UPARROW;               // game: forward
        case M9_KEY_DOWN: return K_DOWNARROW;           // game: back
        case M9_KEY_LEFT: return K_LEFTARROW;           // game: turn left
        case M9_KEY_RIGHT: return K_RIGHTARROW;         // game: turn right
        case M9_KEY_ENTER: return inGame ? K_CTRL : K_ENTER;     // game: fire
        case M9_KEY_DEL:
        case M9_KEY_DEL_LONG: return typing ? K_BACKSPACE : K_ESCAPE;
        case M9_KEY_HOME: return K_ESCAPE;              // menu
        case M9_KEY_MUTE: return K_TAB;                 // scores
        case M9_KEY_TIME: return '`';                   // console
        case M9_KEY_FM: return '/';                     // next weapon
        case M9_KEY_LIGHT:
        case M9_KEY_FM_LONG:
        case M9_KEY_PRESET:
        case M9_KEY_INVALID: return 0;
        default: break;
    }
    if (code < 0x20 || code > 0x7E)
        return 0;
    if (code >= 'A' && code <= 'Z')
        code = code - 'A' + 'a';
    if (inGame)
    {
        switch (code)
        {
            case 'w': return K_UPARROW;     // forward
            case 's': return K_DOWNARROW;   // back
            case 'a': return ',';           // strafe left
            case 'd': return '.';           // strafe right
            case 'q': return '/';           // next weapon
            case 'e': return K_CTRL;        // fire
            default: break;
        }
    }
    return (uint8_t) code;
}

static void inputTask(void *arg)
{
    (void) arg;
    if (!keyboardInit())
    {
        vTaskDeleteWithCaps(NULL);
    }
    uint8_t heldKey = 0;
    int64_t releaseTime = 0;
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(KEYBOARD_POLL_MS));
        int64_t now = esp_timer_get_time();
        int code = readKeyCode();
        if (code > 0)
        {
#if KEYBOARD_DEBUG
            ESP_LOGI(TAG, "key code 0x%02X at %lld ms", code, now / 1000);
#endif
            uint8_t key = gameKey(code);
            if (heldKey && key != heldKey)
            {
                inputPushKey(heldKey, false);
                heldKey = 0;
            }
            if (key)
            {
                // pressing the same key again extends the press
                if (!heldKey)
                    inputPushKey(key, true);
                heldKey = key;
                bool move = key_dest == key_game && (key == K_UPARROW || key == K_DOWNARROW ||
                    key == K_LEFTARROW || key == K_RIGHTARROW || key == ',' || key == '.');
                releaseTime = now + (move ? KEYBOARD_MOVE_TAP_MS : KEYBOARD_TAP_MS) * 1000;
            }
        }
        if (heldKey && now >= releaseTime)
        {
            inputPushKey(heldKey, false);
            heldKey = 0;
        }
    }
}

void boardInputInit(void)
{
    // stack in PSRAM: this task never writes to flash
    xTaskCreatePinnedToCoreWithCaps(inputTask, "m9_input", 3072, NULL, 4, NULL, 0, MALLOC_CAP_SPIRAM);
}
#endif
