/**
 * @file    LVGLDriver.h
 * @brief   LVGL 9.x display driver for LilyGO T-Deck Plus (TFT_eSPI backend)
 *
 * LVGL 9 replaced the lv_disp_drv_t registration pattern with
 * lv_display_create() + lv_display_set_flush_cb(). This file targets that API.
 *
 * Usage
 * -----
 *   Call lvgl_driver_init(tft) after tft.begin() and the splashscreen,
 *   before any lv_* widget calls.
 *   Call lv_timer_handler() + delay(5) in loop().
 */

#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "esp_timer.h"

// ── Buffer sizing ─────────────────────────────────────────────────────────────
// 32 lines × 320 px × 2 bytes = 20 KB per buffer, 40 KB total.
// Safe for internal DRAM on the ESP32-S3. Increase if you add PSRAM.
#ifndef LVGL_BUF_LINES
  #define LVGL_BUF_LINES 32
#endif
#define LVGL_BUF_PX (320 * LVGL_BUF_LINES)

namespace _lvgl_drv {

    static TFT_eSPI* _tft = nullptr;

    // Two draw buffers — LVGL renders into one while we flush the other
    static lv_color_t _buf1[LVGL_BUF_PX];
    static lv_color_t _buf2[LVGL_BUF_PX];

    // ── Flush callback (LVGL 9 signature) ────────────────────────────────────
    // Called by LVGL when a screen region is ready to send to the display.
    // px_map is already in the display's native RGB565 format (lv_color_t = 16-bit).
    static void _flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
        uint32_t w = area->x2 - area->x1 + 1;
        uint32_t h = area->y2 - area->y1 + 1;

        _tft->startWrite();
        _tft->setAddrWindow(area->x1, area->y1, w, h);
        _tft->pushPixels((uint16_t*)px_map, w * h);
        _tft->endWrite();

        lv_display_flush_ready(disp);   // tell LVGL this region is done
    }

    // ── 1 ms tick via ESP32 hardware timer ───────────────────────────────────
    static void IRAM_ATTR _tick_cb(void*) {
        lv_tick_inc(1);
    }

} // namespace _lvgl_drv

/**
 * Initialize LVGL 9 and register the TFT_eSPI display.
 * Call once, after tft.begin() + splashscreen, before creating widgets.
 */
inline void lvgl_driver_init(TFT_eSPI& tft) {
    using namespace _lvgl_drv;
    _tft = &tft;

    // 1. Core LVGL init
    lv_init();

    // 2. Create display object and tell LVGL its resolution
    lv_display_t* disp = lv_display_create(tft.width(), tft.height());

    // 3. Hand LVGL our two buffers (double-buffered partial mode)
    lv_display_set_buffers(disp, _buf1, _buf2,
                           sizeof(_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 4. Register the flush callback
    lv_display_set_flush_cb(disp, _flush_cb);

    //Set color order to BGR and not RGB
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);

    // 5. 1 ms hardware tick — independent of loop() timing
    esp_timer_handle_t tick_timer;
    const esp_timer_create_args_t tick_args = {
        .callback              = _tick_cb,
        .arg                   = nullptr,
        .dispatch_method       = ESP_TIMER_TASK,
        .name                  = "lv_tick",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000)); // 1000 µs = 1 ms

    Serial.println("LVGL 9 driver initialized.");
}