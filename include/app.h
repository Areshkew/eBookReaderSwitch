#ifndef EBOOK_READER_APP_H
#define EBOOK_READER_APP_H

#include <SDL2/SDL.h>
#include <switch.h>
#include "config_manager.h"
#include "theme.h"

struct App {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    PadState pad = {};

    ConfigManager config;
    ThemeManager* themes = nullptr;

    bool darkMode() const { return themes ? themes->IsDark() : true; }
    void setDarkMode(bool) {} 
    bool nightMode() const { return config.Settings().night_mode; }
    void setNightMode(bool v) { config.MutableSettings().night_mode = v; config.MarkDirty(); }
    const Theme& theme() const { return themes->CurrentTheme(); }

    bool imguiInitialized = false;
    bool romfsInited = false;
    bool sdlInited = false;
    bool timeInited = false;
    bool imgInited = false;
    bool ttfInited = false;

    bool init();
    void shutdown();
    ~App() { shutdown(); }

    App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;
};

#endif
