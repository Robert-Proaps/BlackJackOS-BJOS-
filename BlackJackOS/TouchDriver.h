/**
 * @file    TouchDriver.h
 * @brief   GT911 capacitive touch driver for BlackJack-OS
 *
 * Polling-based — reads GT911 over I2C on every LVGL tick.
 * Includes proper hardware reset sequence and a phantom-touch watchdog
 * that resets the chip if it reports the same coordinates for too long.
 *
 * Dependencies
 * ────────────
 *   GT911 by TAMCTec  (install via Arduino Library Manager)
 *
 * Usage
 * ─────
 *   Call touch_driver_init() in systemStartup() after lvgl_driver_init().
 *   Nothing needed in loop().
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include <lvgl.h>
#include "utilities.h"

#define TOUCH_RAW_W  240
#define TOUCH_RAW_H  320

// How many consecutive identical readings before we assume a phantom touch
// and reset the chip. At ~5ms per tick this is about 500ms.
#define TOUCH_PHANTOM_LIMIT  100

namespace _touch_drv {

    static TAMC_GT911 _touch(BOARD_I2C_SDA, BOARD_I2C_SCL,
                              BOARD_TOUCH_INT, BOARD_TOUCH_INT,
                              TOUCH_RAW_W, TOUCH_RAW_H);

    static int16_t _last_x     = 0;
    static int16_t _last_y     = 0;
    static int16_t _prev_raw_x = -1;
    static int16_t _prev_raw_y = -1;
    static int     _same_count = 0;

    // ── Hardware reset sequence ───────────────────────────────────────────────
    // The GT911 needs RST pulsed low then high to re-initialize.
    // Since the T-Deck has no dedicated RST pin we use the INT pin,
    // then release it back to input so the chip can drive it again.
    static void _hw_reset() {
            // Step 1 — drive INT low FIRST to select address 0x5D (or high for 0x14)
        pinMode(BOARD_TOUCH_INT, OUTPUT);
        digitalWrite(BOARD_TOUCH_INT, LOW);   // hold INT low = selects 0x5D
        delay(5);

    // Step 2 — assert RST low (via INT since no dedicated RST pin)
    // On T-Deck this is the same pin, which is the core constraint you're working around
    // You need to assert RST through whatever mechanism you have
    // If INT is your only lever, this sequence is the closest you can get:
        digitalWrite(BOARD_TOUCH_INT, LOW);
        delay(10);

    // Step 3 — release RST (bring high), keep INT low during address latch window
        digitalWrite(BOARD_TOUCH_INT, HIGH);
        delayMicroseconds(110);   // GT911 latches address within 100us of RST rising edge
                               // INT must be stable during this window

    // Step 4 — release INT to input AFTER the latch window
        pinMode(BOARD_TOUCH_INT, INPUT_PULLUP);
        delay(50);
        _touch.begin();
        _prev_raw_x = -1;
        _prev_raw_y = -1;
        _same_count = 0;
        Serial.println("[Touch] GT911 reset complete.");
    }

    static void _correct(int16_t raw_x, int16_t raw_y,
                         int16_t& out_x, int16_t& out_y) {
        out_x = (TOUCH_RAW_H - 1) - raw_y;
        out_y = raw_x;
    }

    static void _read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
        _touch.read();

        if (_touch.isTouched && _touch.touches > 0) {
            int16_t rx = _touch.points[0].x;
            int16_t ry = _touch.points[0].y;

            // Phantom touch watchdog — if raw coordinates haven't changed
            // for TOUCH_PHANTOM_LIMIT consecutive reads, reset the chip
            if (rx == _prev_raw_x && ry == _prev_raw_y) {
                _same_count++;
                if (_same_count >= TOUCH_PHANTOM_LIMIT) {
                    _hw_reset();
                    data->point.x = _last_x;
                    data->point.y = _last_y;
                    data->state   = LV_INDEV_STATE_RELEASED;
                    return;
                }
            } else {
                _same_count = 0;
            }

            _prev_raw_x = rx;
            _prev_raw_y = ry;

            int16_t cx, cy;
            _correct(rx, ry, cx, cy);

            Serial.printf("[Touch] raw(%d,%d) -> corrected(%d,%d)\n", rx, ry, cx, cy);

            _last_x = cx;
            _last_y = cy;

            data->point.x = cx;
            data->point.y = cy;
            data->state   = LV_INDEV_STATE_PRESSED;
        } else {
            // Clean release — reset watchdog
            _prev_raw_x = -1;
            _prev_raw_y = -1;
            _same_count = 0;

            data->point.x = _last_x;
            data->point.y = _last_y;
            data->state   = LV_INDEV_STATE_RELEASED;
        }
    }

} // namespace _touch_drv


inline void touch_driver_init() {
    using namespace _touch_drv;


    // Hardware reset before begin() to ensure clean chip state
    _hw_reset();

    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, _read_cb);

    Serial.println("[Touch] GT911 initialized.");
}

inline bool touch_get(int16_t& x, int16_t& y) {
    x = _touch_drv::_last_x;
    y = _touch_drv::_last_y;
    return _touch_drv::_touch.isTouched;
}