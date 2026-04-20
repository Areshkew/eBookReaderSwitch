#include <stdlib.h>
#include <stdio.h>
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
    #include "MenuChooser.h"
    #include "menu_book_reader.h"
    #include "fs.h"
    #include "config.h"
    #include "paths.h"
    #include "logger.h"
}

SDL_Renderer* RENDERER;
SDL_Window* WINDOW;
SDL_Event EVENT;
TTF_Font *ROBOTO_35, *ROBOTO_30, *ROBOTO_27, *ROBOTO_25, *ROBOTO_20, *ROBOTO_15;
bool configDarkMode;
PadState g_pad;
static bool g_imguiInitialized = false;

void Term_Services() {
    LOG_I("Terminating...");
    if (g_imguiInitialized) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui::DestroyContext();
        g_imguiInitialized = false;
    }
    timeExit();
    if (ROBOTO_35) { TTF_CloseFont(ROBOTO_35); ROBOTO_35 = NULL; }
    if (ROBOTO_30) { TTF_CloseFont(ROBOTO_30); ROBOTO_30 = NULL; }
    if (ROBOTO_27) { TTF_CloseFont(ROBOTO_27); ROBOTO_27 = NULL; }
    if (ROBOTO_25) { TTF_CloseFont(ROBOTO_25); ROBOTO_25 = NULL; }
    if (ROBOTO_20) { TTF_CloseFont(ROBOTO_20); ROBOTO_20 = NULL; }
    if (ROBOTO_15) { TTF_CloseFont(ROBOTO_15); ROBOTO_15 = NULL; }
    TTF_Quit();
    Textures_Free();
    romfsExit();
    IMG_Quit();
    if (RENDERER) { SDL_DestroyRenderer(RENDERER); RENDERER = NULL; }
    if (WINDOW) { SDL_DestroyWindow(WINDOW); WINDOW = NULL; }
    SDL_Quit();
    #ifdef DEBUG
        twiliExit();
    #endif
    Log_Term();
}

bool Init_Services() {
    #ifdef DEBUG
        twiliInitialize();
    #endif

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);

    Result sdmc = fsdevMountSdmc();
    (void)sdmc;

    Log_Init();

    LOG_I("1. romfsInit...");
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        LOG_E("FAILED (0x%X)", rc);
        Term_Services();
        return false;
    }
    LOG_I("OK");

    LOG_I("2. SDL_Init...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0) {
        LOG_E("FAILED (%s)", SDL_GetError());
        Term_Services();
        return false;
    }
    LOG_I("OK");

    LOG_I("3. timeInitialize...");
    timeInitialize();
    LOG_I("OK");

    LOG_I("4. CreateWindow...");
    if (SDL_CreateWindowAndRenderer(1280, 720, 0, &WINDOW, &RENDERER) == -1)  {
        LOG_E("FAILED (%s)", SDL_GetError());
        Term_Services();
        return false;
    }
    LOG_I("OK");

    SDL_SetRenderDrawBlendMode(RENDERER, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    LOG_I("5. IMG_Init...");
    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
        LOG_E("FAILED (%s)", IMG_GetError());
        Term_Services();
        return false;
    }
    LOG_I("OK");

    LOG_I("6. TTF_Init...");
    if (TTF_Init() == -1) {
        LOG_E("FAILED (%s)", TTF_GetError());
        Term_Services();
        return false;
    }
    LOG_I("OK");

    LOG_I("7. Textures_Load...");
    Textures_Load();
    LOG_I("OK");

    LOG_I("8. Fonts....");
    LOG_I("  Checking font file...");
    FILE* fTest = fopen("romfs:/resources/font/Roboto-Light.ttf", "rb");
    if (fTest) {
        LOG_I("  Font file exists");
        fclose(fTest);
    } else {
        LOG_E("FAILED: Font file not found");
        Term_Services();
        return false;
    }
    LOG_I("  Loading Roboto-Light 35...");
    ROBOTO_35 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 35);
    if (!ROBOTO_35) {
        LOG_E("FAILED: Could not open Roboto-Light.ttf");
        Term_Services();
        return false;
    }
    LOG_I("  Loading Roboto-Light 30...");
    ROBOTO_30 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 30);
    LOG_I("  Loading Roboto-Light 27...");
    ROBOTO_27 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 27);
    LOG_I("  Loading Roboto-Light 25...");
    ROBOTO_25 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 25);
    LOG_I("  Loading Roboto-Light 20...");
    ROBOTO_20 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 20);
    LOG_I("  Loading Roboto-Light 15...");
    ROBOTO_15 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 15);
    LOG_I("  All fonts loaded");

    LOG_I("9. Joysticks...");
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            LOG_E("FAILED (%s)", SDL_GetError());
            Term_Services();
            return false;
        }
    }
    LOG_I("OK");

    LOG_I("10. FS_Dirs...");
    if (FS_RecursiveMakeDir(BOOKS_DIR) != 0) {
        LOG_W("Failed to create books directory: %s", BOOKS_DIR);
    }
    LOG_I("OK");

    LOG_I("11. ImGui...");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange; // Switch has no mouse cursor
    io.IniFilename = nullptr; // Don't write imgui.ini to disk
    io.LogFilename = nullptr; // Don't write imgui_log.txt

    io.Fonts->Clear();
    LOG_I("  Loading ImGui font...");
    ImFont* imFont = io.Fonts->AddFontFromFileTTF("romfs:/resources/font/Roboto-Light.ttf", 28.0f);
    if (!imFont) {
        LOG_E("Failed to load ImGui font");
        Term_Services();
        return false;
    }
    LOG_I("  Building font atlas...");
    bool fontAtlasBuilt = io.Fonts->Build();
    LOG_I("  Font atlas built: %s", fontAtlasBuilt ? "OK" : "FAILED");

    LOG_I("  Init SDLRenderer2 backend...");
    if (!ImGui_ImplSDLRenderer2_Init(RENDERER)) {
        LOG_E("ImGui_ImplSDLRenderer2_Init failed");
        Term_Services();
        return false;
    }
    g_imguiInitialized = true;
    LOG_I("OK");

    configDarkMode = true;
    LOG_I("=== Init Complete ===");
    return true;
}

int main(int argc, char *argv[]) {
    if (!Init_Services())
        return 1;

    if (argc == 2) {
        LOG_I("Opening book from association: %s", argv[1]);
        Menu_OpenBook(argv[1]);
    } else {
        LOG_I("Starting book chooser...");
        Menu_StartChoosing();
        LOG_I("Book chooser returned");
    }

    Term_Services();
    return 0;
}
