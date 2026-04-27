#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#ifdef DEBUG
    #include <twili.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"

extern "C" {
    #include "common.h"
    #include "textures.h"
    #include "fs.h"
    #include "paths.h"
    #include "logger.h"
}

#include "app.h"
#include <cstdio>
#include <cstring>

SDL_Renderer* RENDERER;
SDL_Window* WINDOW;
TTF_Font *ROBOTO_35, *ROBOTO_30, *ROBOTO_27, *ROBOTO_25, *ROBOTO_20, *ROBOTO_15;

static constexpr int WINDOW_W = 1280;
static constexpr int WINDOW_H = 720;
static constexpr const char* FONT_PATH = "romfs:/resources/font/Roboto-Light.ttf";
static constexpr int JOYSTICK_COUNT = 2;
static constexpr float IMGUI_FONT_SIZE = 28.0f;
static constexpr const char* RENDER_SCALE_QUALITY = "2";

struct FontEntry {
    TTF_Font** target;
    int size;
};

static const FontEntry FONT_ENTRIES[] = {
    { &ROBOTO_35, 35 }, { &ROBOTO_30, 30 }, { &ROBOTO_27, 27 },
    { &ROBOTO_25, 25 }, { &ROBOTO_20, 20 }, { &ROBOTO_15, 15 },
};

static void closeFonts() {
    for (const auto& entry : FONT_ENTRIES) {
        if (*entry.target) {
            TTF_CloseFont(*entry.target);
            *entry.target = nullptr;
        }
    }
}

static bool loadFonts() {
    for (const auto& entry : FONT_ENTRIES) {
        *entry.target = TTF_OpenFont(FONT_PATH, entry.size);
        if (!*entry.target) {
            LOG_E("Failed to load font size %d", entry.size);
            closeFonts();
            return false;
        }
    }
    return true;
}

void App::shutdown() {
    LOG_I("Terminating...");

    if (config.is_dirty()) {
        config.save();
    }

    if (themes) {
        ThemeManager::shutdown();
        themes = nullptr;
    }

    if (imguiInitialized) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
    }

    if (timeInited) { timeExit(); timeInited = false; }

    closeFonts();

    if (ttfInited) { TTF_Quit(); ttfInited = false; }

    Textures_Free();

    if (romfsInited) { romfsExit(); romfsInited = false; }

    if (imgInited) { IMG_Quit(); imgInited = false; }

    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (window) { SDL_DestroyWindow(window); window = nullptr; }
    RENDERER = nullptr;
    WINDOW = nullptr;

    if (sdlInited) { SDL_Quit(); sdlInited = false; }

    #ifdef DEBUG
        twiliExit();
    #endif

    Log_Term();
}

bool App::init() {
    #ifdef DEBUG
        twiliInitialize();
    #endif

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    Result sdmc = fsdevMountSdmc();
    (void)sdmc;

    Log_Init();

    LOG_I("romfsInit...");
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        LOG_E("FAILED (0x%X)", rc);
        return false;
    }
    romfsInited = true;

    LOG_I("SDL_Init...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0) {
        LOG_E("FAILED (%s)", SDL_GetError());
        return false;
    }
    sdlInited = true;

    LOG_I("timeInitialize...");
    timeInitialize();
    timeInited = true;

    LOG_I("CreateWindow...");
    if (SDL_CreateWindowAndRenderer(WINDOW_W, WINDOW_H, 0, &window, &renderer) == -1) {
        LOG_E("FAILED (%s)", SDL_GetError());
        return false;
    }
    WINDOW = window;
    RENDERER = renderer;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, RENDER_SCALE_QUALITY);

    LOG_I("IMG_Init...");
    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
        LOG_E("FAILED (%s)", IMG_GetError());
        return false;
    }
    imgInited = true;

    LOG_I("TTF_Init...");
    if (TTF_Init() == -1) {
        LOG_E("FAILED (%s)", TTF_GetError());
        return false;
    }
    ttfInited = true;

    LOG_I("Textures_Load...");
    Textures_Load();

    LOG_I("Fonts...");
    if (!loadFonts()) return false;

    LOG_I("Joysticks...");
    for (int i = 0; i < JOYSTICK_COUNT; i++) {
        if (!SDL_JoystickOpen(i)) {
            LOG_E("FAILED (%s)", SDL_GetError());
            return false;
        }
    }

    LOG_I("FS_Dirs...");
    if (FS_RecursiveMakeDir(BOOKS_DIR) != 0) {
        LOG_W("Failed to create books directory: %s", BOOKS_DIR);
    }
    FS_RecursiveMakeDir(THEMES_DIR);

    LOG_I("Config...");
    config.load(CONFIG_FILE);

    LOG_I("Theme...");
    ThemeManager::init();
    themes = &ThemeManager::instance();
    {
        static const char* bundledThemes[] = {"dark", "light"};
        for (const char* tname : bundledThemes) {
            char sd_dir[1024];
            snprintf(sd_dir, sizeof(sd_dir), "%s/%s", THEMES_DIR, tname);
            FS_RecursiveMakeDir(sd_dir);
            char sd_file[2048];
            snprintf(sd_file, sizeof(sd_file), "%s/theme.json", sd_dir);
            if (!FS_FileExists(sd_file)) {
                char romfs_src[1024];
                snprintf(romfs_src, sizeof(romfs_src), "%s/%s/theme.json", THEMES_ROMFS_BASE, tname);
                if (FS_FileExists(romfs_src)) {
                    FILE* src = fopen(romfs_src, "rb");
                    FILE* dst = fopen(sd_file, "wb");
                    if (src && dst) {
                        char buf[4096];
                        size_t n;
                        while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
                            fwrite(buf, 1, n, dst);
                        fclose(src);
                        fclose(dst);
                        LOG_I("Copied default theme: %s", tname);
                    } else {
                        if (src) fclose(src);
                        if (dst) fclose(dst);
                    }
                }
            }
        }
        themes->load_theme(config.settings().active_theme.c_str());
    }

    LOG_I("ImGui...");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.KeyRepeatDelay = FLT_MAX;
    io.KeyRepeatRate = FLT_MAX;

    io.Fonts->Clear();
    ImFont* imFont = io.Fonts->AddFontFromFileTTF(FONT_PATH, IMGUI_FONT_SIZE);
    if (!imFont) {
        LOG_E("Failed to load ImGui font");
        return false;
    }
    io.Fonts->Build();

    LOG_I("ImGui backend...");
    if (!ImGui_ImplSDLRenderer2_Init(renderer)) {
        LOG_E("ImGui_ImplSDLRenderer2_Init failed");
        return false;
    }
    imguiInitialized = true;

    LOG_I("=== Init Complete ===");
    return true;
}
