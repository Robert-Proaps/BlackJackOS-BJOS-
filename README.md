![BJOS_Logo](./Graphics/BJOS_Logo_On_Red.png)

### About The Project
Blackjack-OS is a Free and Open Source Portable Meshcore Toolbox and Platform for the LilyGo T-Deck and T-Deck Plus.

### An Acknowledgment
This project would not be possible without the great work done by the Meshcore developers in the creation of [Meshcore](https://meshcore.io). If you like this project please consider supporting their work.

### AI Usage Disclaimer ✨
Some AI Assisted coding tools were used in a manner limited to boiler-plate structure generation, batch refactoring/reformatting, and exploratory ui development. AI Tools were not used in the implementation of the meshcore standard, and encryption is handeled by readily available libraries. The LLMs used in development include Claude by Anthropic, and Copilot by Microsoft. While all firmware is tested on hardware before being commited to the repository, as the license states it is provided as-is with no warrenty.

## Information for Developers

### Creating New Tools

### Overview

Each app is a self-contained header file that inherits from the `App` base class. The `AppManager` singleton handles launching, screen transitions, and memory cleanup automatically.

---

### Step 1 — Create the App Header

Copy `TemplateApp.h` into your project folder and rename it, e.g. `RadioApp.h`.

Inside the file, find/replace `TemplateApp` with your new class name (e.g. `RadioApp`), then build your UI inside `onLaunch()`.

### Lifecycle hooks available to override

| Hook | When it's called | Typical use |
|---|---|---|
| `onLaunch()` | Once when the app first opens | Create LVGL screen and widgets |
| `onResume()` | When returning from a child app | Refresh dynamic content |
| `onPause()` | When another app takes focus | Pause timers or animations |
| `onDestroy()` | Just before the app is removed | Free any heap allocations |

### Rules to follow in `onLaunch()`

- Always create your screen with `_screen = lv_obj_create(NULL)` — do **not** use `lv_scr_act()`
- Build all child widgets as children of `_screen`
- Always include a back button wired to `AppManager::instance().goHome()`

### Minimal example

```cpp
#pragma once
#include <lvgl.h>
#include "AppManager.h"

class RadioApp : public App {
public:
    const char* name() const override { return "Radio"; }

    void onLaunch() override {
        _screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(_screen, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(_screen, LV_OPA_COVER, LV_PART_MAIN);

        // Your UI here...

        // Back button — always include this
        lv_obj_t* btn = lv_button_create(_screen);
        lv_obj_set_size(btn, 90, 36);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 8, -8);
        lv_obj_add_event_cb(btn, [](lv_event_t*){
            AppManager::instance().goHome();
        }, LV_EVENT_CLICKED, nullptr);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, LV_SYMBOL_LEFT " Back");
        lv_obj_center(lbl);
    }

    void onDestroy() override {
        // Free any heap allocations made in onLaunch() here
        // Do NOT delete _screen — AppManager handles that
    }
};
```

---

## Step 2 — Include the Header in `BlackJackOS.ino`

Add your new header near the top of `BlackJackOS.ino` with the other app includes:

```cpp
#include "TemplateApp.h"
#include "RadioApp.h"    // <-- add this
```

---

## Step 3 — Register a Tile on the Home Menu

In `systemStartup()`, find the `home->addTile()` block and add a new line:

```cpp
home->addTile("Radio", LV_SYMBOL_WIFI, []{ AppManager::instance().launch(new RadioApp()); });
```

The three arguments are:

- **Label** — short text shown under the icon, keep it under ~8 characters
- **Symbol** — an LVGL symbol constant for the icon
- **Lambda** — launches your app via `AppManager`

### Available LVGL symbols

| Symbol constant | Icon |
|---|---|
| `LV_SYMBOL_WIFI` | WiFi / Radio |
| `LV_SYMBOL_GPS` | GPS / Location |
| `LV_SYMBOL_SETTINGS` | Settings gear |
| `LV_SYMBOL_ENVELOPE` | Messages |
| `LV_SYMBOL_BATTERY_FULL` | Battery |
| `LV_SYMBOL_SD_CARD` | SD Card |
| `LV_SYMBOL_BELL` | Notifications |
| `LV_SYMBOL_PLAY` | Play / Run |
| `LV_SYMBOL_LIST` | List / Log |
| `LV_SYMBOL_HOME` | Home |

A full list is available in the [LVGL documentation](https://docs.lvgl.io/master/overview/font.html#built-in-symbols).

---

## Summary

```
1. Copy TemplateApp.h → YourApp.h, rename the class, build your UI
2. #include "YourApp.h" in BlackJackOS.ino
3. home->addTile("Name", LV_SYMBOL_X, []{ AppManager::instance().launch(new YourApp()); });
```
