/**
 * @file    PacketSender.h
 * @brief   Starter template for a BlackJack-OS application
 *
 * Copy this file, rename the class, implement onLaunch() and onDestroy(),
 * then add a tile in BlackJackOS.ino.
 *
 * Quick checklist
 * ───────────────
 *  [x] Rename the class  (search/replace "PacketBuilder")
 *  [x] Set _screen = lv_obj_create(NULL)  in onLaunch()
 *  [x] Build your UI on top of _screen
 *  [x] Add a "Back" button that calls AppManager::instance().goHome()
 *  [x] Free any heap allocations in onDestroy()
 */

#pragma once
#include <lvgl.h>
#include "AppManager.h"
#include <Arduino.h>
#include <lvgl.h>
#include <RadioLib.h>

// ── Forward-declare the radio object from BlackJackOS.ino ─────────────────────
extern SX1262 LoRaRadio;

class PacketBuilder : public App {
public:
    const char* name() const override { return "Template"; }

    // ── Called once when the app is launched ──────────────────────────────────
    void onLaunch() override {
        // 1. Create a new LVGL screen (do NOT use lv_scr_act() — that's for the
        //    active screen; you need a fresh off-screen object)
        _screen = lv_obj_create(NULL);

        // 2. Style the background
        lv_obj_set_style_bg_color(_screen, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

        // ── Your UI starts here ──────────────────────────────────────────────

        // Example: title label
        lv_obj_t* title = lv_label_create(_screen);
        lv_label_set_text(title, "Template App");
        lv_obj_set_style_text_color(title, lv_color_hex(0x00B4D8), LV_PART_MAIN);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

        _buildSendButton();

        

        // ── Back button (always include this) ────────────────────────────────
        _buildBackButton();
    }

    // ── Called when returning from a child app (if you ever push another) ────
    void onResume() override { }

    // ── Called when another app takes focus ──────────────────────────────────
    void onPause() override { }

    // ── Called just before the screen is deleted ─────────────────────────────
    void onDestroy() override {
        // Free any heap memory you allocated in onLaunch here.
        // Do NOT delete _screen — AppManager handles that.
    }

private:
    // ── Standard back button ─────────────────────────────────────────────────
    void _buildBackButton() {
        lv_obj_t* btn = lv_button_create(_screen);
        lv_obj_set_size(btn, 90, 36);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0F3A6B), LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_LEFT " Back");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xE0E0E0), LV_PART_MAIN);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, [](lv_event_t*){
            AppManager::instance().goHome();
        }, LV_EVENT_CLICKED, nullptr);
    }

    void _buildSendButton() {   //Builds out and sets up the send button.
        lv_obj_t * sendBtn = lv_button_create(_screen);
        lv_obj_t * label = lv_label_create(sendBtn);
        lv_label_set_text(label, "Click me");

        // Center the button
        lv_obj_center(sendBtn);
    
        // Style button green
        lv_obj_set_style_bg_color(sendBtn, lv_color_hex(0x00AA00), LV_PART_MAIN);
    
        lv_obj_add_event_cb(sendBtn, send_btn_event, LV_EVENT_CLICKED, NULL);
    }

    static void send_btn_event(lv_event_t * e) {    //Runs when the send button is pressed.
            lv_event_code_t code = lv_event_get_code(e);
            if (code == LV_EVENT_CLICKED) {
                Serial.println("SEND BUTTON PRESSED");
            }
        }
};
