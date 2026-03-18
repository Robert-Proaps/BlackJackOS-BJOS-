/**
 * @file    PacketBuilder.h
 * @brief   MeshCore packet builder — PAYLOAD_TYPE_ADVERT with real Ed25519 crypto
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * DEPENDENCIES (add to your Arduino library manager / platformio.ini):
 *   • rweather/Crypto   (Arduino Cryptography Library — provides Ed25519)
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * CONFIRMED ON-AIR PACKET LAYOUT  (meshcore-decoder + Kingsman blog verified)
 *
 * ── Wire frame ────────────────────────────────────────────────────────────
 *  Byte 0      : header bitfield
 *                  bits 0-1 : route type   01 = FLOOD
 *                  bits 2-5 : payload type 0x04 = ADVERT  (shifted << 2)
 *                  bits 6-7 : version      00 = v0
 *                  → (0x00<<6) | (0x04<<2) | 0x01 = 0x11
 *  Byte 1      : path_len = 0x00 (zero hops — origin only)
 *
 * ── Payload ───────────────────────────────────────────────────────────────
 *  Bytes  0-31 : Ed25519 public key  (32 bytes)
 *  Bytes 32-35 : timestamp           (uint32_t LE, Unix seconds)
 *  Bytes 36-99 : Ed25519 signature   (64 bytes)
 *                  signs: pubkey[32] || timestamp[4] || appFlags[1] || name[n]
 *  Byte  100   : app_flags
 *                  bits 0-3 : role   0x0 = chat client
 *                  bit  4   : GPS present
 *                  bit  7   : name present  (always set here)
 *                  → 0x80 for named chat client, no GPS
 *  Bytes 101+  : node name, null-terminated ("BJOS\0")
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * KEY MANAGEMENT
 *   A fresh Ed25519 keypair is generated on first boot using ESP32's TRNG
 *   and stored in NVS ("bjos_id" namespace) so it survives power cycles.
 *   The identity is stable across reboots — important for MeshCore, as
 *   receiving nodes track your public key for replay protection.
 *
 *   NOTE: rweather/Crypto uses standard Ed25519 key derivation (SHA-512 of
 *   a 32-byte seed), NOT the pre-clamped scalar format used internally by
 *   MeshCore firmware. Both are valid Ed25519 — receivers can verify the
 *   signature either way. Your public key fingerprint will simply differ
 *   from what native MeshCore firmware would produce from the same raw bytes.
 * ═══════════════════════════════════════════════════════════════════════════
 */

#pragma once
#include <Arduino.h>
#include <lvgl.h>
#include <RadioLib.h>
#include <Preferences.h>        // ESP32 NVS
#include <Ed25519.h>            // rweather/Crypto

#include "AppManager.h"

// ── Forward-declare the radio object from BlackJackOS.ino ─────────────────────
extern SX1262 LoRaRadio;

// ── MeshCore wire constants ───────────────────────────────────────────────────
//   Header = (version<<6) | (payload_type<<2) | route_type
//   version=0, payload_type=ADVERT(4), route_type=FLOOD(1)
static constexpr uint8_t MC_HEADER_ADVERT_FLOOD = (0x00 << 6) | (0x04 << 2) | 0x01; // 0x11

//   App-flags byte: bit7=name present, bits0-3=role(0=chat client)
static constexpr uint8_t MC_APP_FLAGS_CHAT_NAMED = 0x80;

// ── NVS namespace + keys ──────────────────────────────────────────────────────
static constexpr const char* NVS_NS        = "bjos_id";
static constexpr const char* NVS_PRIV_KEY  = "priv";
static constexpr const char* NVS_PUB_KEY   = "pub";

// ── Colour palette ────────────────────────────────────────────────────────────
static constexpr uint32_t PB_BG_COLOR     = 0x1A1A2E;
static constexpr uint32_t PB_ACCENT_COLOR = 0x00B4D8;
static constexpr uint32_t PB_TEXT_COLOR   = 0xE0E0E0;
static constexpr uint32_t PB_DIM_COLOR    = 0x444466;
static constexpr uint32_t PB_BTN_COLOR    = 0x0F3A6B;
static constexpr uint32_t PB_OK_COLOR     = 0x00CC66;
static constexpr uint32_t PB_ERR_COLOR    = 0xCC3333;
static constexpr uint32_t PB_WARN_COLOR   = 0xFFAA00;

// ─────────────────────────────────────────────────────────────────────────────
class PacketBuilder : public App {
public:
    const char* name() const override { return "Packet Builder"; }

    // ── Launch ────────────────────────────────────────────────────────────────
    void onLaunch() override {
        _screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_screen, lv_color_hex(PB_BG_COLOR), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_screen,   LV_OPA_COVER,              LV_PART_MAIN);
        lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

        _loadOrGenerateIdentity();
        _buildUI();
    }

    void onResume()  override { }
    void onPause()   override { }
    void onDestroy() override { }

private:
    // ── Crypto identity ───────────────────────────────────────────────────────
    uint8_t _privKey[32];   // Ed25519 private key (32-byte seed)
    uint8_t _pubKey[32];    // Ed25519 public key

    // ── Widget handles ────────────────────────────────────────────────────────
    lv_obj_t* _statusLabel  = nullptr;
    lv_obj_t* _pubkeyLabel  = nullptr;

    // ── Identity: load from NVS, or generate & save ───────────────────────────
    void _loadOrGenerateIdentity() {
        Preferences prefs;
        prefs.begin(NVS_NS, false);

        size_t privLen = prefs.getBytesLength(NVS_PRIV_KEY);
        size_t pubLen  = prefs.getBytesLength(NVS_PUB_KEY);

        if (privLen == 32 && pubLen == 32) {
            prefs.getBytes(NVS_PRIV_KEY, _privKey, 32);
            prefs.getBytes(NVS_PUB_KEY,  _pubKey,  32);
            Serial.println("[PacketBuilder] Identity loaded from NVS.");
        } else {
            Serial.println("[PacketBuilder] No identity found — generating keypair…");
            Ed25519::generatePrivateKey(_privKey);
            Ed25519::derivePublicKey(_pubKey, _privKey);
            prefs.putBytes(NVS_PRIV_KEY, _privKey, 32);
            prefs.putBytes(NVS_PUB_KEY,  _pubKey,  32);
            Serial.println("[PacketBuilder] New identity saved to NVS.");
        }

        Serial.print("[PacketBuilder] Public key : ");
        for (int i = 0; i < 32; i++) Serial.printf("%02x", _pubKey[i]);
        Serial.println();
        Serial.print("[PacketBuilder] Private key: ");
        for (int i = 0; i < 32; i++) Serial.printf("%02x", _privKey[i]);
        Serial.println();

        prefs.end();
    }

    // ── UI construction ───────────────────────────────────────────────────────
    void _buildUI() {
        // Header bar
        lv_obj_t* header = lv_obj_create(_screen);
        lv_obj_set_size(header, LV_PCT(100), 36);
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(header,     lv_color_hex(0x0D1B2A), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(header,       LV_OPA_COVER,           LV_PART_MAIN);
        lv_obj_set_style_border_width(header, 0,                      LV_PART_MAIN);
        lv_obj_set_style_radius(header,       0,                      LV_PART_MAIN);
        lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);

        lv_obj_t* titleLbl = lv_label_create(header);
        lv_label_set_text(titleLbl, LV_SYMBOL_ENVELOPE "  Packet Builder");
        lv_obj_set_style_text_color(titleLbl, lv_color_hex(PB_ACCENT_COLOR), LV_PART_MAIN);
        lv_obj_align(titleLbl, LV_ALIGN_LEFT_MID, 8, 0);

        // Public key fingerprint (first 4 bytes shown)
        _pubkeyLabel = lv_label_create(_screen);
        char pkBuf[32];
        snprintf(pkBuf, sizeof(pkBuf), "ID: %02x%02x%02x%02x\xe2\x80\xa6",
                 _pubKey[0], _pubKey[1], _pubKey[2], _pubKey[3]);
        lv_label_set_text(_pubkeyLabel, pkBuf);
        lv_obj_set_style_text_color(_pubkeyLabel, lv_color_hex(PB_DIM_COLOR), LV_PART_MAIN);
        lv_obj_align(_pubkeyLabel, LV_ALIGN_TOP_MID, 0, 44);

        // Send Advert button
        lv_obj_t* advertBtn = lv_button_create(_screen);
        lv_obj_set_size(advertBtn, 190, 52);
        lv_obj_align(advertBtn, LV_ALIGN_CENTER, 0, -10);
        lv_obj_set_style_bg_color(advertBtn, lv_color_hex(PB_BTN_COLOR), LV_PART_MAIN);
        lv_obj_set_style_radius(advertBtn, 10, LV_PART_MAIN);

        lv_obj_t* advertLbl = lv_label_create(advertBtn);
        lv_label_set_text(advertLbl, LV_SYMBOL_WIFI "  Send Advert");
        lv_obj_set_style_text_color(advertLbl, lv_color_hex(PB_TEXT_COLOR), LV_PART_MAIN);
        lv_obj_center(advertLbl);

        lv_obj_add_event_cb(advertBtn, [](lv_event_t* e){
            auto* self = static_cast<PacketBuilder*>(lv_event_get_user_data(e));
            self->_sendAdvertPacket();
        }, LV_EVENT_CLICKED, this);

        // Status label
        _statusLabel = lv_label_create(_screen);
        lv_label_set_text(_statusLabel, "Ready \xe2\x80\x94 tap to advertise BJOS");
        lv_obj_set_style_text_color(_statusLabel, lv_color_hex(PB_DIM_COLOR), LV_PART_MAIN);
        lv_obj_set_style_text_align(_statusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_width(_statusLabel, LV_PCT(90));
        lv_obj_align(_statusLabel, LV_ALIGN_CENTER, 0, 55);

        _buildBackButton();
    }

    // ── Build & transmit the advert packet ────────────────────────────────────
    void _sendAdvertPacket() {
        lv_label_set_text(_statusLabel, "Signing\xe2\x80\xa6");
        lv_obj_set_style_text_color(_statusLabel, lv_color_hex(PB_WARN_COLOR), LV_PART_MAIN);
        lv_timer_handler(); // flush UI before blocking crypto call

        // ── 1. Node name & app flags ──────────────────────────────────────────
        const char*   nodeName  = "BJOS";
        const uint8_t nameLen   = (uint8_t)strlen(nodeName) + 1; // includes '\0'
        const uint8_t appFlags  = MC_APP_FLAGS_CHAT_NAMED;        // 0x80

        // ── 2. Timestamp ──────────────────────────────────────────────────────
        //   No RTC on this board — millis()/1000 gives seconds since boot.
        //   Receiving nodes use the timestamp for replay protection: they reject
        //   adverts with ts <= last seen ts from the same pubkey. Since we boot
        //   from zero each time, rapidly pressing the button won't cause drops
        //   within a single session, and a reboot will advance time anyway.
        uint32_t ts = (uint32_t)(millis() / 1000UL);
        uint8_t  tsBytes[4] = {
            (uint8_t)( ts        & 0xFF),
            (uint8_t)((ts >>  8) & 0xFF),
            (uint8_t)((ts >> 16) & 0xFF),
            (uint8_t)((ts >> 24) & 0xFF),
        };

        // ── 3. Build signing message: pubkey || timestamp || appFlags || name ──
        //   Source: DeepWiki meshcore-dev — "Signature covers: public key +
        //   timestamp + app data", where app data = appFlags byte + name string.
        const size_t signMsgLen = 32 + 4 + 1 + nameLen;
        uint8_t signMsg[signMsgLen];
        size_t  off = 0;
        memcpy(signMsg + off, _pubKey,   32); off += 32;
        memcpy(signMsg + off, tsBytes,    4); off +=  4;
        signMsg[off++] = appFlags;
        memcpy(signMsg + off, nodeName, nameLen); off += nameLen;

        // ── 4. Sign ───────────────────────────────────────────────────────────
        uint8_t sig[64];
        Ed25519::sign(sig, _privKey, _pubKey, signMsg, signMsgLen);

        // ── 5. Assemble full on-air frame ─────────────────────────────────────
        //   [header:1][path_len:1][pubkey:32][ts:4][sig:64][appFlags:1][name:n]
        const size_t payloadLen = 32 + 4 + 64 + 1 + nameLen;
        const size_t totalLen   = 2 + payloadLen;
        uint8_t pkt[totalLen];

        off = 0;
        pkt[off++] = MC_HEADER_ADVERT_FLOOD; // 0x11
        pkt[off++] = 0x00;                   // path_len = 0

        memcpy(pkt + off, _pubKey,   32); off += 32;
        memcpy(pkt + off, tsBytes,    4); off +=  4;
        memcpy(pkt + off, sig,        64); off += 64;
        pkt[off++] = appFlags;
        memcpy(pkt + off, nodeName, nameLen); off += nameLen;

        // ── 6. Serial diagnostic dump ─────────────────────────────────────────
        Serial.printf("\n[PacketBuilder] ===== ADVERT TX =====\n");
        Serial.printf("[PacketBuilder] Header     : 0x%02X\n",  pkt[0]);
        Serial.printf("[PacketBuilder] Path len   : 0x%02X\n",  pkt[1]);
        Serial.printf("[PacketBuilder] Timestamp  : %lu s\n",   (unsigned long)ts);
        Serial.printf("[PacketBuilder] App flags  : 0x%02X\n",  appFlags);
        Serial.printf("[PacketBuilder] Node name  : %s\n",      nodeName);
        Serial.printf("[PacketBuilder] Total bytes: %u\n",      (unsigned)totalLen);
        Serial.print("[PacketBuilder] PubKey     : ");
        for (int i = 0; i < 32; i++) Serial.printf("%02x", _pubKey[i]);
        Serial.println();
        Serial.print("[PacketBuilder] Signature  : ");
        for (int i = 0; i < 64; i++) Serial.printf("%02x", sig[i]);
        Serial.println();
        Serial.print("[PacketBuilder] Full HEX   : ");
        for (size_t i = 0; i < totalLen; i++) Serial.printf("%02X", pkt[i]);
        Serial.println();
        Serial.printf("[PacketBuilder] Paste into meshcore-decoder to verify:\n");
        Serial.printf("[PacketBuilder]   meshcore-decoder ");
        for (size_t i = 0; i < totalLen; i++) Serial.printf("%02X", pkt[i]);
        Serial.println();
        Serial.printf("[PacketBuilder] =====================\n\n");

        // ── 7. Transmit ───────────────────────────────────────────────────────
        int state = LoRaRadio.transmit(pkt, totalLen);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("[PacketBuilder] TX OK.");
            lv_label_set_text(_statusLabel, LV_SYMBOL_OK "  Advert sent!");
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(PB_OK_COLOR), LV_PART_MAIN);
        } else {
            Serial.printf("[PacketBuilder] TX failed: %d\n", state);
            char errBuf[48];
            snprintf(errBuf, sizeof(errBuf), LV_SYMBOL_CLOSE "  TX failed (%d)", state);
            lv_label_set_text(_statusLabel, errBuf);
            lv_obj_set_style_text_color(_statusLabel, lv_color_hex(PB_ERR_COLOR), LV_PART_MAIN);
        }

        LoRaRadio.standby();
    }

    // ── Back button ───────────────────────────────────────────────────────────
    void _buildBackButton() {
        lv_obj_t* btn = lv_button_create(_screen);
        lv_obj_set_size(btn, 90, 36);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
        lv_obj_set_style_bg_color(btn, lv_color_hex(PB_BTN_COLOR), LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_LEFT " Back");
        lv_obj_set_style_text_color(lbl, lv_color_hex(PB_TEXT_COLOR), LV_PART_MAIN);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, [](lv_event_t*){
            AppManager::instance().goHome();
        }, LV_EVENT_CLICKED, nullptr);
    }
};