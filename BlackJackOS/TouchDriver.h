/**
 * @file    TouchDriver.h
 * @brief   CST816S capacitive touch driver for BlackJack-OS
 *
 * Provides:
 *   1. LVGL lv_indev registration — all widgets respond to touch automatically.
 *   2. TouchDriver::getTouch(x, y) — raw corrected coordinates for custom use.
 *
 * Dependencies
 * ────────────
 *   Arduino-CST816S by fcsobral  (install via Arduino Library Manager)
 *
 * Usage
 * ─────
 *   Call touch_driver_init() in systemStartup() after lvgl_driver_init().
 *   Nothing needed in loop() — LVGL polls the indev via lv_timer_handler().
 *
 * Coordinate correction
 * ─────────────────────
 *   TFT rotation(1) = landscape. The CST816S reports portrait coordinates,
 *   so raw (x, y) must be transformed:
 *       corrected_x = raw_y
 *       corrected_y = (TOUCH_RAW_W - 1) - raw_x
 *   where TOUCH_RAW_W is the panel's portrait width (240 px on the T-Deck).
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <CST816S.h>
#include <lvgl.h>
#include "utilities.h"

// Portrait resolution of the CST816S panel (before rotation correction)
#define TOUCH_RAW_W  240
#define TOUCH_RAW_H  320

namespace _touch_drv {

    static CST816S _touch(BOARD_I2C_SDA, BOARD_I2C_SCL,
                          BOARD_TOUCH_INT, BOARD_TOUCH_INT); // rst reused as int

    // Flag set by ISR, cleared after each LVGL read
    static volatile bool _touched = false;

    // Last corrected coordinates (written in ISR context, read in LVGL callback)
    static volatile int16_t _last_x = 0;
    static volatile int16_t _last_y = 0;

    // ── Interrupt service routine ─────────────────────────────────────────────
    static void IRAM_ATTR _touch_isr() {
        _touched = true;
    }

    // ── Coordinate correction (portrait → landscape rotation 1) ──────────────
    static void _correct(int16_t raw_x, int16_t raw_y,
                         int16_t& out_x, int16_t& out_y) {
        out_x = raw_y;
        out_y = (TOUCH_RAW_W - 1) - raw_x;
    }

    // ── LVGL input device read callback ──────────────────────────────────────
    static void _read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
        if (_touched) {
            // Read the CST816 over I2C
            if (_touch.available()) {
                int16_t cx, cy;
                _correct(_touch.data.x, _touch.data.y, cx, cy);

                _last_x = cx;
                _last_y = cy;

                data->point.x = cx;
                data->point.y = cy;
                data->state   = LV_INDEV_STATE_PRESSED;
            } else {
                // Interrupt fired but no data ready yet — report last position
                data->point.x = _last_x;
                data->point.y = _last_y;
                data->state   = LV_INDEV_STATE_RELEASED;
                _touched = false;
            }
        } else {
            data->point.x = _last_x;
            data->point.y = _last_y;
            data->state   = LV_INDEV_STATE_RELEASED;
        }
    }

} // namespace _touch_drv


// ─────────────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Initialize the CST816S and register it with LVGL.
 * Call once in systemStartup(), after lvgl_driver_init().
 */
inline void touch_driver_init() {
    using namespace _touch_drv;

    // Start I2C on the board's dedicated touch pins
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);


    // Attach falling-edge interrupt on the INT pin
    pinMode(BOARD_TOUCH_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BOARD_TOUCH_INT),
                    _touch_isr, FALLING);

    // Register with LVGL as a pointer input device
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, _read_cb);

    Serial.println("[Touch] CST816S initialized.");
}

/**
 * Read the last corrected touch position.
 * Safe to call from anywhere — does NOT talk to I2C, reads cached values.
 *
 * @param x  Out: corrected X (0…319 in landscape)
 * @param y  Out: corrected Y (0…239 in landscape)
 * @return   true if a touch is currently active, false if finger is up
 */
inline bool touch_get(int16_t& x, int16_t& y) {
    x = _touch_drv::_last_x;
    y = _touch_drv::_last_y;
    return _touch_drv::_touched;
}
