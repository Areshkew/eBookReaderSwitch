#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#ifdef DEBUG
    #include <twili.h>
#endif

extern "C" {
    #include "common.h"
    #include "textures.h"
    #include "MenuChooser.h"
    #include "menu_book_reader.h"
    #include "fs.h"
    #include "config.h"
}

SDL_Renderer* RENDERER;
SDL_Window* WINDOW;
SDL_Event EVENT;
TTF_Font *ROBOTO_35, *ROBOTO_30, *ROBOTO_27, *ROBOTO_25, *ROBOTO_20, *ROBOTO_15;
bool configDarkMode;

FILE* g_logFile = NULL;
PadState g_pad;
bool g_consoleActive = true;

void Log(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    printf("%s\n", buf);
    if (g_consoleActive) consoleUpdate(NULL);
    if (g_logFile) {
        fprintf(g_logFile, "%s\n", buf);
        fflush(g_logFile);
    }
}

void WaitAuto() {
    Log("Waiting 2 seconds...");
    SDL_Delay(2000);
}

void Term_Services() {
    Log("Terminating...");
    timeExit();
    TTF_CloseFont(ROBOTO_35); TTF_CloseFont(ROBOTO_30); TTF_CloseFont(ROBOTO_27);
    TTF_CloseFont(ROBOTO_25); TTF_CloseFont(ROBOTO_20); TTF_CloseFont(ROBOTO_15);
    TTF_Quit();
    Textures_Free();
    romfsExit();
    IMG_Quit();
    SDL_DestroyRenderer(RENDERER);
    SDL_DestroyWindow(WINDOW);
    SDL_Quit();
    #ifdef DEBUG
        twiliExit();
    #endif
    if (g_logFile) fclose(g_logFile);
}

void Init_Services() {
    #ifdef DEBUG
        twiliInitialize();
    #endif

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);

    Result sdmc = fsdevMountSdmc();
    g_logFile = fopen("sdmc:/eBookReader_debug.log", "w");
    if (g_logFile) Log("Log file opened.");
    else Log("WARNING: Log file failed to open!");

    Log("1. romfsInit...");
    Result rc = romfsInit();
    if (R_FAILED(rc)) Log("FAILED (0x%X)", rc); else Log("OK");

    Log("2. SDL_Init...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0) {
        Log("FAILED (%s)", SDL_GetError());
        Term_Services();
    }
    Log("OK");

    Log("3. timeInitialize...");
    timeInitialize();
    Log("OK");

    Log("4. CreateWindow...");
    if (SDL_CreateWindowAndRenderer(1280, 720, 0, &WINDOW, &RENDERER) == -1)  {
        Log("FAILED (%s)", SDL_GetError());
        Term_Services();
    }
    Log("OK");
    WaitAuto();

    SDL_SetRenderDrawBlendMode(RENDERER, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

    Log("5. IMG_Init...");
    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
        Log("FAILED (%s)", IMG_GetError());
        Term_Services();
    }
    Log("OK");
    WaitAuto();

    Log("6. TTF_Init...");
    if(TTF_Init() == -1) {
        Log("FAILED (%s)", TTF_GetError());
        Term_Services();
    }
    Log("OK");
    WaitAuto();

    Log("7. Textures_Load...");
    Textures_Load();
    Log("OK");
    WaitAuto();

    Log("8. Fonts....");
    Log("  Checking font file...");
    FILE* fTest = fopen("romfs:/resources/font/Roboto-Light.ttf", "rb");
    if (fTest) { Log("  Font file exists"); fclose(fTest); }
    else { Log("FAILED: Font file not found"); Term_Services(); }
    Log("  Loading Roboto-Light 35...");
    ROBOTO_35 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 35);
    if (!ROBOTO_35) { Log("FAILED: Could not open Roboto-Light.ttf"); Term_Services(); }
    Log("  Loading Roboto-Light 30...");
    ROBOTO_30 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 30);
    Log("  Loading Roboto-Light 27...");
    ROBOTO_27 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 27);
    Log("  Loading Roboto-Light 25...");
    ROBOTO_25 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 25);
    Log("  Loading Roboto-Light 20...");
    ROBOTO_20 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 20);
    Log("  Loading Roboto-Light 15...");
    ROBOTO_15 = TTF_OpenFont("romfs:/resources/font/Roboto-Light.ttf", 15);
    Log("  All fonts loaded");
    WaitAuto();
    
    Log("9. Joysticks...");
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            Log("FAILED (%s)", SDL_GetError());
            Term_Services();
        }
    }
    Log("OK");
    WaitAuto();

    Log("10. FS_Dirs...");
    FS_RecursiveMakeDir("/switch/eBookReader/books");
    Log("OK");
    WaitAuto();

    configDarkMode = true;
    Log("=== Init Complete ===");
}

int main(int argc, char *argv[]) {
    Init_Services();

    if (argc == 2) {
        Menu_OpenBook(argv[1]);
    } else {
        Menu_StartChoosing();
    }

    Term_Services();
    return 0;
}
