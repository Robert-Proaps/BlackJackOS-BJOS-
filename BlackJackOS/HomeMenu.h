/**
 * @file    HomeMenu.h
 * @brief   Home screen for BlackJack-OS
 *
 * Displays a grid of labelled icon tiles.
 * Each tile is registered with addTile() before launch.
 *
 * Usage
 * ─────
 *   // In systemStartup(), after lvgl_driver_init():
 *
 *   HomeMenu* home = new HomeMenu();
 *   home->addTile("Radio",    LV_SYMBOL_WIFI,   []{ AppManager::instance().launch(new RadioApp()); });
 *   home->addTile("Settings", LV_SYMBOL_SETTINGS, []{ AppManager::instance().launch(new SettingsApp()); });
 *   AppManager::instance().setHome(home);
 */

#pragma once
#include <lvgl.h>
#include <functional>
#include <vector>
#include <string>
#include "AppManager.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Theming constants  (tweak to taste)
// ─────────────────────────────────────────────────────────────────────────────
namespace HomeTheme {
    // Background
    static constexpr lv_color_t BG_COLOR      = LV_COLOR_MAKE(0x1A, 0x1A, 0x2E);
    // Tile face
    static constexpr lv_color_t TILE_BG       = LV_COLOR_MAKE(0x16, 0x21, 0x3E);
    static constexpr lv_color_t TILE_BORDER   = LV_COLOR_MAKE(0x0F, 0x3A, 0x6B);
    static constexpr lv_color_t TILE_PRESSED  = LV_COLOR_MAKE(0x0F, 0x3A, 0x6B);
    // Text / icon
    static constexpr lv_color_t ICON_COLOR    = LV_COLOR_MAKE(0x00, 0xB4, 0xD8);
    static constexpr lv_color_t LABEL_COLOR   = LV_COLOR_MAKE(0xE0, 0xE0, 0xE0);
    // Header bar
    static constexpr lv_color_t HEADER_COLOR  = LV_COLOR_MAKE(0x0D, 0x1B, 0x2A);

    static constexpr uint8_t  TILE_RADIUS     = 12;   // px, corner rounding
    static constexpr uint8_t  TILE_BORDER_W   = 2;    // px
    static constexpr uint16_t TILE_SIZE       = 80;   // px square
    static constexpr uint8_t  TILE_PAD        = 12;   // gap between tiles
    static constexpr uint8_t  ICON_FONT_SIZE  = 30;   // approx — uses LV_FONT_MONTSERRAT_30
}

// ─────────────────────────────────────────────────────────────────────────────
//  HomeMenu
// ─────────────────────────────────────────────────────────────────────────────
class HomeMenu : public App {
public:
    // Tile descriptor
    struct Tile {
        std::string              label;
        const char*              symbol;   // LVGL symbol string, e.g. LV_SYMBOL_WIFI
        std::function<void()>    onTap;
    };

    const char* name() const override { return "Home"; }

    /**
     * Register a tile before setHome() is called.
     * @param label   Short text shown below the icon (keep ≤ 8 chars)
     * @param symbol  LVGL symbol constant, e.g. LV_SYMBOL_SETTINGS
     * @param onTap   Callback — typically launches an App via AppManager
     */
    void addTile(const char* label, const char* symbol, std::function<void()> onTap) {
        _tiles.push_back({ label, symbol, std::move(onTap) });
    }

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    void onLaunch() override {
        _screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_screen, HomeTheme::BG_COLOR, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_screen,   LV_OPA_COVER,        LV_PART_MAIN);
        lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

        _buildHeader();
        _buildGrid();
    }

    void onResume() override {
        // Nothing dynamic to refresh right now — extend here (e.g. update clock)
    }

    void onDestroy() override {
        // _screen deleted by AppManager; no extra heap to free
        _tiles.clear();
    }

private:
    std::vector<Tile> _tiles;

    // ── Header bar ────────────────────────────────────────────────────────────
    void _buildHeader() {
        lv_obj_t* header = lv_obj_create(_screen);
        lv_obj_set_size(header, LV_PCT(100), 36);
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(header,   HomeTheme::HEADER_COLOR, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(header,     LV_OPA_COVER,            LV_PART_MAIN);
        lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(header,       0, LV_PART_MAIN);
        lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);

        lv_obj_t* title = lv_label_create(header);
        lv_label_set_text(title, LV_SYMBOL_HOME "  BlackJack OS");
        lv_obj_set_style_text_color(title, HomeTheme::ICON_COLOR, LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_LEFT_MID, 8, 0);
    }

    // ── Tile grid ─────────────────────────────────────────────────────────────
    void _buildGrid() {
        // Flex container — wraps tiles automatically
        lv_obj_t* grid = lv_obj_create(_screen);
        lv_obj_set_style_bg_opa(grid,       LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(grid, 0,             LV_PART_MAIN);
        lv_obj_set_style_pad_all(grid,      HomeTheme::TILE_PAD, LV_PART_MAIN);
        lv_obj_set_style_pad_gap(grid,      HomeTheme::TILE_PAD, LV_PART_MAIN);

        // Occupy screen below the header
        lv_obj_set_pos(grid, 0, 36);
        lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100) - 36);

        lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(grid,    LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(grid,   LV_FLEX_ALIGN_START,
                                      LV_FLEX_ALIGN_CENTER,
                                      LV_FLEX_ALIGN_START);
        lv_obj_set_scroll_dir(grid, LV_DIR_VER);

        for (size_t i = 0; i < _tiles.size(); ++i) {
            _makeTile(grid, _tiles[i]);
        }
    }

    // ── Single tile ───────────────────────────────────────────────────────────
    void _makeTile(lv_obj_t* parent, const Tile& tile) {
        lv_obj_t* btn = lv_obj_create(parent);
        lv_obj_set_size(btn, HomeTheme::TILE_SIZE, HomeTheme::TILE_SIZE);
        lv_obj_set_style_bg_color(btn,     HomeTheme::TILE_BG,     LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn,       LV_OPA_COVER,           LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, HomeTheme::TILE_BORDER, LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, HomeTheme::TILE_BORDER_W, LV_PART_MAIN);
        lv_obj_set_style_radius(btn,       HomeTheme::TILE_RADIUS,  LV_PART_MAIN);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

        // Pressed state
        lv_obj_set_style_bg_color(btn, HomeTheme::TILE_PRESSED, LV_PART_MAIN | LV_STATE_PRESSED);

        // Icon (symbol)
        lv_obj_t* icon = lv_label_create(btn);
        lv_label_set_text(icon, tile.symbol);
        lv_obj_set_style_text_color(icon, HomeTheme::ICON_COLOR, LV_PART_MAIN);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -10);

        // Label
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, tile.label.c_str());
        lv_obj_set_style_text_color(lbl, HomeTheme::LABEL_COLOR, LV_PART_MAIN);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 18);

        // Touch callback — capture the tile's onTap by index via user_data
        // We store a heap copy of the std::function so the lambda outlives this scope
        auto* cb = new std::function<void()>(tile.onTap);
        lv_obj_set_user_data(btn, cb);
        lv_obj_add_event_cb(btn, [](lv_event_t* e){
            auto* fn = static_cast<std::function<void()>*>(
                           lv_event_get_user_data(e));
            if (fn && *fn) (*fn)();
        }, LV_EVENT_CLICKED, cb);
    }
};
