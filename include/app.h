#ifndef EBOOK_READER_APP_H
#define EBOOK_READER_APP_H

#include <SDL2/SDL.h>
#include <switch.h>

struct App {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool darkMode = true;
    bool nightMode = false;
    PadState pad = {};

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
