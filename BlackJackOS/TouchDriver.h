/**
 * @file    TouchDriver.h
 * @brief   GT911 capacitive touch driver for BlackJack-OS
 *
 * Polling-based — reads GT911 over I2C on every LVGL tick.
 * No ISR, no race conditions. Simple and reliable.
 *
 * Dependencies
 * ────────────
 *   GT911 by TAMCTec  (install via Arduino Library Manager)
 *
 * Usage
 * ─────
 *   Call touch_driver_init() in systemStartup() after lvgl_driver_init().
 *   Nothing needed in loop().
 *
 * Coordinate correction
 * ─────────────────────
 *   TFT rotation(1) = landscape. GT911 reports portrait coordinates.
 *       corrected_x = raw_y
 *       corrected_y = (TOUCH_RAW_W - 1) - raw_x
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <TAMC_GT911.h>
#include <lvgl.h>
#include "utilities.h"

#define TOUCH_RAW_W  240
#define TOUCH_RAW_H  320

namespace _touch_drv {

    static TAMC_GT911 _touch(BOARD_I2C_SDA, BOARD_I2C_SCL,
                              BOARD_TOUCH_INT, BOARD_TOUCH_INT,
                              TOUCH_RAW_W, TOUCH_RAW_H);

    static int16_t _last_x = 0;
    static int16_t _last_y = 0;

    static void _correct(int16_t raw_x, int16_t raw_y,
                         int16_t& out_x, int16_t& out_y) {
        out_x = (TOUCH_RAW_H - 1) - raw_y;
        out_y = raw_x;
    }

    static void _read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
        _touch.read();

        if (_touch.isTouched && _touch.touches > 0) {
            int16_t cx, cy;
            _correct(_touch.points[0].x, _touch.points[0].y, cx, cy);

            Serial.printf("[Touch] raw(%d,%d) -> corrected(%d,%d)\n",
                          _touch.points[0].x, _touch.points[0].y, cx, cy);

            _last_x = cx;
            _last_y = cy;

            data->point.x = cx;
            data->point.y = cy;
            data->state   = LV_INDEV_STATE_PRESSED;
        } else {
            data->point.x = _last_x;
            data->point.y = _last_y;
            data->state   = LV_INDEV_STATE_RELEASED;
        }
    }

} // namespace _touch_drv


inline void touch_driver_init() {
    using namespace _touch_drv;

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    _touch.begin();

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