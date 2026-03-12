#pragma once
#include <TFT_eSPI.h>
#include "utilities.h"

// ─────────────────────────────────────────────
//  Cursor shape  (1 = draw, 0 = transparent)
//  Edit this bitmap to change the cursor appearance.
//  CURSOR_W / CURSOR_H must match the array dimensions.
// ─────────────────────────────────────────────
#define CURSOR_W  11
#define CURSOR_H  11

static const uint8_t CURSOR_SHAPE[CURSOR_H][CURSOR_W] = {
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
    { 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0 },
};

// ─────────────────────────────────────────────
//  Trackball ISR state  (volatile, shared with ISRs)
// ─────────────────────────────────────────────
volatile int16_t _tb_dx = 0;
volatile int16_t _tb_dy = 0;

void IRAM_ATTR _tb_up()    { _tb_dy--; }
void IRAM_ATTR _tb_down()  { _tb_dy++; }
void IRAM_ATTR _tb_left()  { _tb_dx--; }
void IRAM_ATTR _tb_right() { _tb_dx++; }

// ─────────────────────────────────────────────
//  Cursor
// ─────────────────────────────────────────────
class Cursor {
public:
    // bgColor  – color used to erase the cursor (should match your app background)
    // fgColor  – color used to draw the cursor
    Cursor(TFT_eSPI& display,
           uint16_t  fgColor = TFT_WHITE,
           uint16_t  bgColor = TFT_BLACK)
        : _tft(display),
          _fg(fgColor),
          _bg(bgColor),
          _x(display.width()  / 2),
          _y(display.height() / 2),
          _visible(false)
    {}

    // Call once during setup() — after tft.begin() and setRotation()
    void begin() {
        pinMode(BOARD_TBOX_G01, INPUT_PULLUP);   // up
        pinMode(BOARD_TBOX_G02, INPUT_PULLUP);   // right
        pinMode(BOARD_TBOX_G03, INPUT_PULLUP);   // down
        pinMode(BOARD_TBOX_G04, INPUT_PULLUP);   // left

        attachInterrupt(digitalPinToInterrupt(BOARD_TBOX_G01), _tb_up,    FALLING);
        attachInterrupt(digitalPinToInterrupt(BOARD_TBOX_G03), _tb_down,  FALLING);
        attachInterrupt(digitalPinToInterrupt(BOARD_TBOX_G04), _tb_left,  FALLING);
        attachInterrupt(digitalPinToInterrupt(BOARD_TBOX_G02), _tb_right, FALLING);
    }

    // Call every loop() iteration.
    // Returns true if the cursor actually moved (caller can decide to redraw).
    bool update() {
        // Atomically snapshot and clear the ISR deltas
        noInterrupts();
        int16_t dx = _tb_dx;
        int16_t dy = _tb_dy;
        _tb_dx = 0;
        _tb_dy = 0;
        interrupts();

        if (dx == 0 && dy == 0) return false;

        erase();

        // Clamp to screen bounds
        _x = constrain(_x + dx, 0, _tft.width()  - CURSOR_W - 1);
        _y = constrain(_y + dy, 0, _tft.height() - CURSOR_H - 1);

        draw();
        return true;
    }

    // Force a full redraw at current position (e.g. after screen refresh)
    void draw() {
        _render(_fg);
        _visible = true;
    }

    // Erase by repainting the cursor area in the background color
    void erase() {
        if (_visible) _render(_bg);
        _visible = false;
    }

    // Change colors mid-session (e.g. app theme changes)
    void setColors(uint16_t fg, uint16_t bg) { _fg = fg; _bg = bg; }

    int16_t x() const { return _x; }
    int16_t y() const { return _y; }

private:
    void _render(uint16_t color) {
        for (uint8_t row = 0; row < CURSOR_H; row++) {
            for (uint8_t col = 0; col < CURSOR_W; col++) {
                if (CURSOR_SHAPE[row][col]) {
                    _tft.drawPixel(_x + col, _y + row, color);
                }
            }
        }
    }

    TFT_eSPI& _tft;
    uint16_t  _fg;
    uint16_t  _bg;
    int16_t   _x;
    int16_t   _y;
    bool      _visible;
};
