/**
 * @file    TouchDriver.h
 * @brief   GT911 capacitive touch driver for BlackJack-OS
 *
 * Uses lewisxhe's SensorLib (TouchDrvGT911) instead of TAMC_GT911.
 * SensorLib correctly handles the GT911 INT-based address-latch sequence
 * and accepts RST = -1 for hardware configurations (like the T-Deck) where
 * no dedicated RST line is available. This resolves intermittent boot failures
 * caused by the broken RST/INT shared-pin reset sequence in the prior driver.
 *
 * Dependencies
 * ────────────
 *   SensorLib by lewisxhe  (install via Arduino Library Manager)
 *   https://github.com/lewisxhe/SensorLib
 *
 * Usage
 * ─────
 *   Call touch_driver_init() in systemStartup() after lvgl_driver_init().
 *   Wire.begin() is called internally — do NOT call it again elsewhere.
 *   Nothing needed in loop().
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <TouchDrvGT911.hpp> //From Sensor Lib by Lewis He
#include <lvgl.h>
#include "utilities.h"

// Native GT911 resolution (portrait). The _correct() transform rotates to
// landscape to match the display orientation set in systemStartup().
#define TOUCH_RAW_W  240
#define TOUCH_RAW_H  320

// How many consecutive identical readings before we assume a phantom touch.
// At ~5 ms per LVGL tick this is approximately 500 ms.
#define TOUCH_PHANTOM_LIMIT  100

namespace _touch_drv {

    static TouchDrvGT911 _touch;

    static int16_t _last_x     =  0;
    static int16_t _last_y     =  0;
    static int16_t _prev_raw_x = -1;
    static int16_t _prev_raw_y = -1;
    static int     _same_count =  0;

    // ── Coordinate correction ─────────────────────────────────────────────────
    // The GT911 reports in portrait (240 × 320). The display is rotated to
    // landscape (rotation 1), so we remap:
    //   corrected_x = (RAW_H - 1) - raw_y
    //   corrected_y = raw_x
    static void _correct(int16_t raw_x, int16_t raw_y,
                         int16_t& out_x, int16_t& out_y) {
        out_x = raw_y;
        out_y = (TOUCH_RAW_W - 1) - raw_x;
    }

    // ── LVGL input-device read callback ──────────────────────────────────────
    static void _read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
        int16_t x[1], y[1];
        uint8_t touched = _touch.getPoint(x, y, 1);

        if (touched > 0) {
            int16_t rx = x[0];
            int16_t ry = y[0];

            // Phantom-touch watchdog ─ if raw coordinates are frozen for
            // TOUCH_PHANTOM_LIMIT consecutive reads, discard the event.
            // SensorLib's cleaner reset sequence makes true phantoms rare,
            // but the guard is kept as a safety net.
            if (rx == _prev_raw_x && ry == _prev_raw_y) {
                if (++_same_count >= TOUCH_PHANTOM_LIMIT) {
                    Serial.println("[Touch] Phantom touch detected — discarding.");
                    _same_count   = 0;
                    _prev_raw_x   = -1;
                    _prev_raw_y   = -1;
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

            Serial.printf("[Touch] raw(%d,%d) -> corrected(%d,%d)\n",
                          rx, ry, cx, cy);

            _last_x = cx;
            _last_y = cy;

            data->point.x = cx;
            data->point.y = cy;
            data->state   = LV_INDEV_STATE_PRESSED;

        } else {
            // Clean release — reset watchdog state
            _prev_raw_x   = -1;
            _prev_raw_y   = -1;
            _same_count   = 0;
            data->point.x = _last_x;
            data->point.y = _last_y;
            data->state   = LV_INDEV_STATE_RELEASED;
        }
    }

} // namespace _touch_drv


// ── Public init function ──────────────────────────────────────────────────────
inline void touch_driver_init() {
    using namespace _touch_drv;

    // Initialise I2C bus. Safe to call here even if Wire was never started —
    // if you add other I2C devices later, move Wire.begin() to systemStartup()
    // (before this call) and remove it here.
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);

    // RST = -1  →  SensorLib skips the RST pulse and uses only INT-pin timing
    //              for the GT911 I2C address-latch sequence. This is the correct
    //              approach for the T-Deck where RST is not broken out separately.
    // INT = BOARD_TOUCH_INT (GPIO 16)
    _touch.setPins(-1, BOARD_TOUCH_INT);

    // GT911_SLAVE_ADDRESS2 = 0x5D, selected when INT is held low at power-on.
    // This matches the T-Deck's pull-down on the INT line at boot.
    if (!_touch.begin(Wire, GT911_SLAVE_ADDRESS_H)) {
        Serial.println("[Touch] ERROR — GT911 not found on I2C bus. "
                       "Check power, wiring, and boot-time settling delay.");
        return;
    }

    _touch.setMaxCoordinates(TOUCH_RAW_W, TOUCH_RAW_H);

    // Register LVGL input device
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, _read_cb);

    Serial.println("[Touch] GT911 initialized via SensorLib.");
}


// ── Optional helper — last known touch position ───────────────────────────────
// Returns true if the GT911 currently reports a touch.
inline bool touch_get(int16_t& x, int16_t& y) {
    x = _touch_drv::_last_x;
    y = _touch_drv::_last_y;
    int16_t tx[1], ty[1];
    return (_touch_drv::_touch.getPoint(tx, ty, 1) > 0);
}