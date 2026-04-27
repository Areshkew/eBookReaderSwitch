extern "C" {
    #include "logger.h"
}

#include "app.h"
#include "menu_book_reader.h"
#include "menu_chooser.h"
#include "book_reader.hpp"

#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_switch.h"

#include <cmath>
#include <string>

struct TouchTracker {
    HidTouchScreenState prevState = {};
    int pinchCooldown = 0;
    int maxTouchCount = 0;
    bool isDragging = false;
    bool inUIBar = false;
    float dragStartX = 0, dragStartY = 0;
    float prevX = 0, prevY = 0;
    bool hadTouch = false;
};

static TouchTracker s_tracker;
static float s_prevPinchDist = 0.0f;
static bool s_showThemeModal = false;

static float touchDistance(const HidTouchState& a, const HidTouchState& b) {
    float dx = static_cast<float>(a.x) - static_cast<float>(b.x);
    float dy = static_cast<float>(a.y) - static_cast<float>(b.y);
    return std::sqrt(dx * dx + dy * dy);
}

static bool pointInUIBar(float x, float y, bool showUI, BookPageLayout layout) {
    if (!showUI) return false;
    if (layout == BookPageLayoutPortrait) {
        const float topBarH = 55.0f;
        const float bottomBarH = 82.0f;
        const float displayH = 720.0f;
        return (y < topBarH) || (y > (displayH - bottomBarH));
    } else {
        const float landBarW = 110.0f;
        const float displayW = 1280.0f;
        return (x < landBarW) || (x > (displayW - landBarW));
    }
}

static void processTap(BookReader* reader, float x, float y) {
    if (reader->showUI) {
        if (!pointInUIBar(x, y, true, reader->currentPageLayout())) {
            reader->showUI = false;
        }
    } else {
        if (reader->currentPageLayout() == BookPageLayoutPortrait) {
            if (y < 108.0f) {
                reader->showUI = true;
            } else if (x < 427.0f) {
                reader->previous_page(1);
            } else if (x > 853.0f) {
                reader->next_page(1);
            }
        } else {
            const float landBarW = 110.0f;
            const float displayW = 1280.0f;
            if (x > (displayW - landBarW)) {
                reader->showUI = true;
            } else if (y < 240.0f && x > landBarW) {
                reader->previous_page(1);
            } else if (y > 480.0f && x > landBarW) {
                reader->next_page(1);
            }
        }
    }
}

void Menu_OpenBook(App& app, const char* path) {
    BookReader *reader = NULL;
    int result = 0;

    reader = new BookReader(app, path, &result);

    if (result < 0) {
        LOG_W("Menu_OpenBook: document not loaded");
    }

    hidInitializeTouchScreen();

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    while (result >= 0 && appletMainLoop()) {
        ImGuiProcessSDLEvents();

        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        u64 kHeld = padGetButtons(&pad);
        u64 kUp   = padGetButtonsUp(&pad);

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        if (io.DeltaTime <= 0.0f || io.DeltaTime > 1.0f)
            io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

        bool enableNav = reader->showUI || s_showThemeModal;
        ImGuiUpdateSwitchInput(&pad, enableNav);
        ImGuiSetSwitchTheme(app.darkMode());

        HidTouchScreenState touchState = {0};
        bool hasTouch = !s_showThemeModal && hidGetTouchScreenStates(&touchState, 1);

        if (hasTouch) {
            s_tracker.maxTouchCount = (touchState.count > s_tracker.maxTouchCount)
                ? touchState.count : s_tracker.maxTouchCount;
        }

        if (hasTouch && touchState.count >= 2) {
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
            s_tracker.pinchCooldown = 20;
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
                if (pointInUIBar(tx, ty, reader->showUI, reader->currentPageLayout())) {
                    s_tracker.inUIBar = true;
                } else {
                    s_tracker.inUIBar = false;

                    if (s_tracker.prevState.count == 0) {
                        s_tracker.dragStartX = tx;
                        s_tracker.dragStartY = ty;
                        s_tracker.prevX = tx;
                        s_tracker.prevY = ty;
                        s_tracker.isDragging = false;
                    } else {
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
            if (s_tracker.hadTouch && s_tracker.prevState.count == 1 &&
                !s_tracker.isDragging && !s_tracker.inUIBar &&
                s_tracker.maxTouchCount < 2) {
                processTap(reader, s_tracker.dragStartX, s_tracker.dragStartY);
            }
            s_tracker.isDragging = false;
            s_tracker.inUIBar = false;
            s_tracker.hadTouch = false;
            s_tracker.maxTouchCount = 0;
            if (s_tracker.pinchCooldown > 0)
                s_tracker.pinchCooldown--;
            s_prevPinchDist = 0.0f;
        }

        s_tracker.prevState = touchState;

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
            s_showThemeModal = true;
        }

        if (kDown & HidNpadButton_Plus) {
            app.setNightMode(!app.nightMode());
        }

        reader->draw();
        if (reader->requestExit) {
            break;
        }

        if (s_showThemeModal) {
            ImGui::OpenPopup("Themes");
        }
        ImVec2 modalCenter = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(modalCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 450));
        if (ImGui::BeginPopupModal("Themes", &s_showThemeModal,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
        {
            auto themeList = app.themes->ListThemes();
            const std::string& active = app.config.Settings().active_theme;
            const Theme& th = app.theme();
            ImVec4 accentCol(th.accent.r / 255.0f, th.accent.g / 255.0f, th.accent.b / 255.0f, th.accent.a / 255.0f);

            ImGui::Text("Select Theme");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 8));

            ImGui::BeginChild("ThemeList", ImVec2(0, -50), ImGuiChildFlags_NavFlattened);
            for (const auto& tname : themeList) {
                bool isSelected = (tname == active);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Text, accentCol);
                }
                if (ImGui::Selectable(tname.c_str(), isSelected, 0, ImVec2(0, 36))) {
                    if (tname != active) {
                        app.themes->LoadTheme(tname.c_str());
                        app.config.MutableSettings().active_theme = tname;
                        app.config.MarkDirty();
                        app.config.Save();
                        reader->rerender_page();
                    }
                    s_showThemeModal = false;
                    ImGui::CloseCurrentPopup();
                }
                if (isSelected) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 4));
            float bw = 120.0f;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - bw) * 0.5f);
            if (ImGui::Button("Close", ImVec2(bw, 36))) {
                s_showThemeModal = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), app.renderer);
        SDL_RenderPresent(app.renderer);
    }

    LOG_I("Exiting reader");
    LOG_I("Opening chooser");
    Menu_StartChoosing(app);
    delete reader;
}
