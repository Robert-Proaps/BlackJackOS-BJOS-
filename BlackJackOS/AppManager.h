/**
 * @file    AppManager.h
 * @brief   Lightweight pseudo-application framework for BlackJack-OS
 *
 * Architecture
 * ────────────
 *  App            – pure-virtual base every "application" inherits from.
 *  AppManager     – singleton that owns the active app and handles transitions.
 *
 * Lifecycle
 * ─────────
 *  launch(app)    – pause current app → call app->onLaunch() → load its screen
 *  goHome()       – destroy current app → resume the home menu
 *
 *  Each app owns exactly one lv_screen (created in onLaunch, deleted in onDestroy).
 *  Screen transitions use lv_screen_load_anim so the display never tears.
 *
 * Usage in an app header
 * ──────────────────────
 *  class MyApp : public App {
 *  public:
 *      const char* name() const override { return "My App"; }
 *      void onLaunch()  override { /* build your LVGL UI here *\/ }
 *      void onResume()  override { }
 *      void onPause()   override { }
 *      void onDestroy() override { /* free resources *\/ }
 *  };
 */

#pragma once
#include <lvgl.h>

// ─────────────────────────────────────────────────────────────────────────────
//  App – base class
// ─────────────────────────────────────────────────────────────────────────────
class App {
public:
    virtual ~App() = default;

    // Human-readable identifier (used for debug prints)
    virtual const char* name() const = 0;

    /**
     * Called once when the app is first launched.
     * Create your LVGL screen here via lv_obj_create(NULL),
     * then store it in _screen. Build all child widgets on top of _screen.
     */
    virtual void onLaunch()  = 0;

    /**
     * Called when returning to this app after another app was on top.
     * Default: no-op. Override to refresh dynamic content (e.g. LoRa stats).
     */
    virtual void onResume()  {}

    /**
     * Called just before another app takes focus.
     * Default: no-op. Override to pause timers / animations.
     */
    virtual void onPause()   {}

    /**
     * Called when the app is being removed from memory.
     * Delete any heap allocations here. The AppManager will
     * call lv_obj_delete(_screen) after this returns.
     */
    virtual void onDestroy() {}

    // The LVGL screen object this app renders to.
    // Must be set during onLaunch() via: _screen = lv_obj_create(NULL);
    lv_obj_t* getScreen() const { return _screen; }

protected:
    lv_obj_t* _screen = nullptr;
};


// ─────────────────────────────────────────────────────────────────────────────
//  AppManager – singleton
// ─────────────────────────────────────────────────────────────────────────────
class AppManager {
public:
    // Access the singleton
    static AppManager& instance() {
        static AppManager inst;
        return inst;
    }

    /**
     * Register the home screen app. Call this once in systemStartup(),
     * after lvgl_driver_init(). The home app is launched immediately.
     *
     * @param home  Heap-allocated home app (AppManager takes ownership).
     */
    void setHome(App* home) {
        _homeApp = home;
        _launch(home, LV_SCR_LOAD_ANIM_NONE, 0);
    }

    /**
     * Launch an app from the home menu (or any other app).
     * The current app is paused (NOT destroyed — home stays alive).
     *
     * @param app   Heap-allocated app. AppManager takes ownership and will
     *              delete it when the user returns home.
     * @param anim  Transition animation (default: slide left).
     */
    void launch(App* app,
                lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_LEFT,
                uint32_t animMs = 250)
    {
        if (_currentApp) _currentApp->onPause();
        _prevApp = _currentApp;
        _launch(app, anim, animMs);
    }

    /**
     * Destroy the current app and return to home.
     * Safe to call from within an app's own callbacks.
     */
    void goHome(lv_scr_load_anim_t anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT,
                uint32_t animMs = 250)
    {
        if (_currentApp == _homeApp) return;   // already home

        App* dying = _currentApp;

        // Load home screen first so the dying app's screen is not active
        // when we delete it
        lv_screen_load_anim(_homeApp->getScreen(), anim, animMs, 0, false);
        _currentApp = _homeApp;
        _homeApp->onResume();

        // Schedule cleanup after the animation completes
        // We use a one-shot LVGL timer so the screen is fully loaded first
        struct _CleanCtx { App* app; };
        auto* ctx = new _CleanCtx{dying};
        lv_timer_t* t = lv_timer_create([](lv_timer_t* tmr){
            auto* c = static_cast<_CleanCtx*>(lv_timer_get_user_data(tmr));
            c->app->onDestroy();
            if (c->app->getScreen()) lv_obj_delete(c->app->getScreen());
            delete c->app;
            delete c;
            lv_timer_delete(tmr);
        }, animMs + 50, ctx);
        (void)t;

        _prevApp = nullptr;
    }

    // Current active app (may be home)
    App* current() const { return _currentApp; }

    // The registered home app
    App* home()    const { return _homeApp; }

private:
    AppManager() = default;
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;

    void _launch(App* app, lv_scr_load_anim_t anim, uint32_t ms) {
        app->onLaunch();
        lv_screen_load_anim(app->getScreen(), anim, ms, 0, false);
        _currentApp = app;
    }

    App* _homeApp    = nullptr;
    App* _currentApp = nullptr;
    App* _prevApp    = nullptr;
};
