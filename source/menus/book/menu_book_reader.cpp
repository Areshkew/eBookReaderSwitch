extern "C" {
    #include "menu_book_reader.h"
    #include "MenuChooser.h"
    #include "common.h"
    #include "config.h"
    #include "SDL_helper.h"
    #include "logger.h"
}

#include "BookReader.hpp"
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_switch.h"

#include <cmath>

// ---- Touch tracking ----
struct TouchTracker {
    HidTouchScreenState prevState = {};
    int pinchCooldown = 0;
    bool isDragging = false;
    bool inUIBar = false;
    float dragStartX = 0, dragStartY = 0;
    float prevX = 0, prevY = 0;
    bool hadTouch = false;
};

static TouchTracker s_tracker;
static float s_prevPinchDist = 0.0f;

static float touchDistance(const HidTouchState& a, const HidTouchState& b) {
    float dx = static_cast<float>(a.x) - static_cast<float>(b.x);
    float dy = static_cast<float>(a.y) - static_cast<float>(b.y);
    return std::sqrt(dx * dx + dy * dy);
}

static bool pointInUIBar(float x, float y, bool showUI) {
    if (!showUI) return false;
    const float topBarH = 55.0f;
    const float bottomBarH = 70.0f;
    const float displayH = 720.0f;
    return (y < topBarH) || (y > (displayH - bottomBarH));
}

static void processTap(BookReader* reader, float x, float y) {
    if (reader->showUI) {
        // Tap anywhere in the page area hides the UI
        if (!pointInUIBar(x, y, true)) {
            reader->showUI = false;
        }
        // Taps inside UI bars are handled by ImGui via SDL events
    } else {
        if (y < 108.0f) {
            reader->showUI = true;
        } else if (x < 427.0f) {
            reader->previous_page(1);
        } else if (x > 853.0f) {
            reader->next_page(1);
        }
    }
}

void Menu_OpenBook(char *path) {
    BookReader *reader = NULL;
    int result = 0;

    reader = new BookReader(path, &result);
    
    if (result < 0) {
        LOG_W("Menu_OpenBook: document not loaded");
    }
    
    hidInitializeTouchScreen();

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    while (result >= 0 && appletMainLoop()) {
        // ---- Feed SDL events to ImGui ----
        ImGuiProcessSDLEvents();

        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        u64 kHeld = padGetButtons(&pad);
        u64 kUp   = padGetButtonsUp(&pad);

        // ---- Begin ImGui frame ----
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        if (io.DeltaTime <= 0.0f || io.DeltaTime > 1.0f)
            io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

        ImGuiUpdateSwitchInput(&pad, reader->showUI);
        ImGuiSetSwitchTheme(configDarkMode);

        // ---- Touch handling ----
        HidTouchScreenState touchState = {0};
        bool hasTouch = hidGetTouchScreenStates(&touchState, 1);

        if (hasTouch && touchState.count >= 2) {
            // ---- Pinch zoom ----
            float dist = touchDistance(touchState.touches[0], touchState.touches[1]);
            if (s_tracker.prevState.count < 2) {
                s_prevPinchDist = dist;
            } else {
                float delta = dist - s_prevPinchDist;
                const float PINCH_THRESHOLD = 20.0f;
                if (delta > PINCH_THRESHOLD) {
                    reader->zoom_by(1.08f);
                    s_prevPinchDist = dist;
                } else if (delta < -PINCH_THRESHOLD) {
                    reader->zoom_by(1.0f / 1.08f);
                    s_prevPinchDist = dist;
                }
            }
            s_tracker.pinchCooldown = 12;
            s_tracker.isDragging = false;
            s_tracker.inUIBar = false;
            s_tracker.hadTouch = true;
        } else if (hasTouch && touchState.count == 1) {
            auto& t = touchState.touches[0];
            float tx = static_cast<float>(t.x);
            float ty = static_cast<float>(t.y);

            if (s_tracker.pinchCooldown > 0) {
                s_tracker.pinchCooldown--;
            } else {
                if (pointInUIBar(tx, ty, reader->showUI)) {
                    s_tracker.inUIBar = true;
                } else {
                    s_tracker.inUIBar = false;

                    if (s_tracker.prevState.count == 0) {
                        // Touch just began
                        s_tracker.dragStartX = tx;
                        s_tracker.dragStartY = ty;
                        s_tracker.prevX = tx;
                        s_tracker.prevY = ty;
                        s_tracker.isDragging = false;
                    } else {
                        // Touch continuing
                        float moveDist = std::sqrt(
                            (tx - s_tracker.dragStartX) * (tx - s_tracker.dragStartX) +
                            (ty - s_tracker.dragStartY) * (ty - s_tracker.dragStartY));
                        const float DRAG_THRESHOLD = 12.0f;

                        if (!s_tracker.isDragging && moveDist > DRAG_THRESHOLD) {
                            s_tracker.isDragging = true;
                        }

                        if (s_tracker.isDragging) {
                            float dx = tx - s_tracker.prevX;
                            float dy = ty - s_tracker.prevY;
                            reader->pan_page(dx, dy);
                            s_tracker.prevX = tx;
                            s_tracker.prevY = ty;
                        }
                    }
                }
            }
            s_tracker.hadTouch = true;
        } else {
            // No touches
            if (s_tracker.hadTouch && s_tracker.prevState.count == 1 &&
                !s_tracker.isDragging && !s_tracker.inUIBar) {
                processTap(reader, s_tracker.dragStartX, s_tracker.dragStartY);
            }
            s_tracker.isDragging = false;
            s_tracker.inUIBar = false;
            s_tracker.hadTouch = false;
            if (s_tracker.pinchCooldown > 0)
                s_tracker.pinchCooldown--;
            s_prevPinchDist = 0.0f;
        }

        s_tracker.prevState = touchState;

        // ---- Button controls (unchanged mappings) ----
        if (kDown & HidNpadButton_Left) {
            if (reader->currentPageLayout() == BookPageLayoutPortrait) {
                reader->previous_page(1);
            } else if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->zoom_out();
            }
        } else if (kDown & HidNpadButton_Right) {
            if (reader->currentPageLayout() == BookPageLayoutPortrait) {
                reader->next_page(1);
            } else if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->zoom_in();
            }
        }

        if (kDown & HidNpadButton_R) {
            reader->next_page(10);
        } else if (kDown & HidNpadButton_L) {
            reader->previous_page(10);
        }

        if ((kDown & HidNpadButton_Up) || (kHeld & HidNpadButton_StickRUp)) {
            if (reader->currentPageLayout() == BookPageLayoutPortrait) {
                reader->zoom_in();
            } else if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->previous_page(1);
            }
        } else if ((kDown & HidNpadButton_Down) || (kHeld & HidNpadButton_StickRDown)) {
            if (reader->currentPageLayout() == BookPageLayoutPortrait) {
                reader->zoom_out();
            } else if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->next_page(1);
            }
        }

        if (kHeld & HidNpadButton_StickLUp) {
            if (reader->currentPageLayout() == BookPageLayoutPortrait) {
                reader->move_page_up();
            } else if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->move_page_right();
            }
        } else if (kHeld & HidNpadButton_StickLDown) {
            if (reader->currentPageLayout() == BookPageLayoutPortrait) {
                reader->move_page_down();
            } else if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->move_page_left();
            }
        } else if (kHeld & HidNpadButton_StickLRight) {
            if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->move_page_down();
            }
        } else if (kHeld & HidNpadButton_StickLLeft) {
            if (reader->currentPageLayout() == BookPageLayoutLandscape) {
                reader->move_page_up();
            }
        }

        if (kDown & HidNpadButton_LeftSR)
            reader->next_page(10);
        else if (kDown & HidNpadButton_LeftSL)
            reader->previous_page(10);

        if (kUp & HidNpadButton_B) {
            break;
        }

        if (kDown & HidNpadButton_X) {
            reader->showUI = !reader->showUI;
        }

        if ((kDown & HidNpadButton_StickL) || (kDown & HidNpadButton_StickR)) {
            reader->reset_page();
        }

        if (kDown & HidNpadButton_Y) {
            reader->switch_page_layout();
        }

        if (kUp & HidNpadButton_Minus) {
            configDarkMode = !configDarkMode;
            reader->previous_page(0);
        }

        if (kDown & HidNpadButton_Plus) {
            reader->showUI = !reader->showUI;
        }

        // ---- Draw page + ImGui overlays ----
        reader->draw();
        if (reader->requestExit) {
            break;
        }

        // ---- Render ImGui on top and present ----
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), RENDERER);
        SDL_RenderPresent(RENDERER);
    }

    LOG_I("Exiting reader");
    LOG_I("Opening chooser");
    Menu_StartChoosing();
    delete reader;
}
