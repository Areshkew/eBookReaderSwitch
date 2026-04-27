#include "imgui_switch.h"
#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"
#include "theme.h"

#include <unordered_map>

static const std::unordered_map<std::string, ImGuiCol_> kImGuiColMap = {
    {"Text",                 ImGuiCol_Text},
    {"TextDisabled",         ImGuiCol_TextDisabled},
    {"WindowBg",             ImGuiCol_WindowBg},
    {"ChildBg",              ImGuiCol_ChildBg},
    {"PopupBg",              ImGuiCol_PopupBg},
    {"Border",               ImGuiCol_Border},
    {"FrameBg",              ImGuiCol_FrameBg},
    {"FrameBgHovered",       ImGuiCol_FrameBgHovered},
    {"FrameBgActive",        ImGuiCol_FrameBgActive},
    {"TitleBg",              ImGuiCol_TitleBg},
    {"TitleBgActive",        ImGuiCol_TitleBgActive},
    {"MenuBarBg",            ImGuiCol_MenuBarBg},
    {"ScrollbarBg",          ImGuiCol_ScrollbarBg},
    {"ScrollbarGrab",        ImGuiCol_ScrollbarGrab},
    {"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
    {"ScrollbarGrabActive",  ImGuiCol_ScrollbarGrabActive},
    {"CheckMark",            ImGuiCol_CheckMark},
    {"SliderGrab",           ImGuiCol_SliderGrab},
    {"SliderGrabActive",     ImGuiCol_SliderGrabActive},
    {"Button",               ImGuiCol_Button},
    {"ButtonHovered",        ImGuiCol_ButtonHovered},
    {"ButtonActive",         ImGuiCol_ButtonActive},
    {"Header",               ImGuiCol_Header},
    {"HeaderHovered",        ImGuiCol_HeaderHovered},
    {"HeaderActive",         ImGuiCol_HeaderActive},
    {"Separator",            ImGuiCol_Separator},
    {"ResizeGrip",           ImGuiCol_ResizeGrip},
    {"ResizeGripHovered",    ImGuiCol_ResizeGripHovered},
    {"ResizeGripActive",     ImGuiCol_ResizeGripActive},
    {"Tab",                  ImGuiCol_Tab},
    {"TabHovered",           ImGuiCol_TabHovered},
    {"TabActive",            ImGuiCol_TabActive},
    {"ModalWindowDimBg",     ImGuiCol_ModalWindowDimBg},
};

void ImGuiSetSwitchTheme(bool dark) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    const Theme& theme = ThemeManager::instance().current_theme();

    if (!theme.imgui_colors.empty()) {
        for (const auto& kv : theme.imgui_colors) {
            auto it = kImGuiColMap.find(kv.first);
            if (it != kImGuiColMap.end()) {
                const ThemeImGuiColor& c = kv.second;
                colors[it->second] = ImVec4(c.r, c.g, c.b, c.a);
            }
        }
    } else {
        if (dark) {
            colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
            colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colors[ImGuiCol_ChildBg]                = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_PopupBg]                = ImVec4(0.15f, 0.15f, 0.15f, 0.98f);
            colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
            colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colors[ImGuiCol_FrameBgActive]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_TitleBg]                = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TitleBgActive]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colors[ImGuiCol_MenuBarBg]              = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
            colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
            colors[ImGuiCol_CheckMark]              = ImVec4(0.00f, 0.82f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrab]             = ImVec4(0.00f, 0.82f, 1.00f, 1.00f);
            colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.20f, 0.90f, 1.00f, 1.00f);
            colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_ButtonHovered]          = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
            colors[ImGuiCol_ButtonActive]           = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            colors[ImGuiCol_Header]                 = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            colors[ImGuiCol_HeaderHovered]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_HeaderActive]           = ImVec4(0.00f, 0.72f, 0.90f, 0.30f);
            colors[ImGuiCol_Separator]              = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
            colors[ImGuiCol_ResizeGrip]             = ImVec4(0.00f, 0.82f, 1.00f, 0.50f);
            colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.00f, 0.82f, 1.00f, 0.75f);
            colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.00f, 0.82f, 1.00f, 1.00f);
            colors[ImGuiCol_Tab]                    = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
            colors[ImGuiCol_TabHovered]             = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colors[ImGuiCol_TabActive]              = ImVec4(0.00f, 0.72f, 0.90f, 1.00f);
            colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
        } else {
            colors[ImGuiCol_Text]                   = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
            colors[ImGuiCol_ChildBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
            colors[ImGuiCol_Border]                 = ImVec4(0.70f, 0.70f, 0.70f, 0.50f);
            colors[ImGuiCol_FrameBg]                = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
            colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
            colors[ImGuiCol_FrameBgActive]          = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
            colors[ImGuiCol_TitleBg]                = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
            colors[ImGuiCol_TitleBgActive]          = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
            colors[ImGuiCol_MenuBarBg]              = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
            colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
            colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colors[ImGuiCol_CheckMark]              = ImVec4(0.00f, 0.55f, 0.75f, 1.00f);
            colors[ImGuiCol_SliderGrab]             = ImVec4(0.00f, 0.55f, 0.75f, 1.00f);
            colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.00f, 0.65f, 0.85f, 1.00f);
            colors[ImGuiCol_Button]                 = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
            colors[ImGuiCol_ButtonHovered]          = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
            colors[ImGuiCol_ButtonActive]           = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
            colors[ImGuiCol_Header]                 = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
            colors[ImGuiCol_HeaderHovered]          = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
            colors[ImGuiCol_HeaderActive]           = ImVec4(0.00f, 0.55f, 0.75f, 0.30f);
            colors[ImGuiCol_Separator]              = ImVec4(0.70f, 0.70f, 0.70f, 0.50f);
            colors[ImGuiCol_ResizeGrip]             = ImVec4(0.00f, 0.55f, 0.75f, 0.50f);
            colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.00f, 0.55f, 0.75f, 0.75f);
            colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.00f, 0.55f, 0.75f, 1.00f);
            colors[ImGuiCol_Tab]                    = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
            colors[ImGuiCol_TabHovered]             = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
            colors[ImGuiCol_TabActive]              = ImVec4(0.00f, 0.55f, 0.75f, 1.00f);
            colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.40f);
        }
    }

    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 6.0f;
    style.ChildRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
    style.WindowBorderSize  = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 0.0f;
    style.ItemSpacing       = ImVec2(8, 8);
    style.FramePadding      = ImVec2(10, 6);
}

void ImGuiUpdateSwitchInput(PadState* pad, bool enableNav) {
    ImGuiIO& io = ImGui::GetIO();
    u64 kDown = padGetButtonsDown(pad);
    u64 kUp   = padGetButtonsUp(pad);

    auto keyEvent = [&](HidNpadButton btn, ImGuiKey key) {
        if (kDown & btn) io.AddKeyEvent(key, true);
        if (kUp   & btn) io.AddKeyEvent(key, false);
    };

    keyEvent(HidNpadButton_A,     ImGuiKey_Enter);
    keyEvent(HidNpadButton_B,     ImGuiKey_Escape);
    keyEvent(HidNpadButton_Plus,  ImGuiKey_Space);
    keyEvent(HidNpadButton_Minus, ImGuiKey_Tab);

    if (enableNav) {
        keyEvent(HidNpadButton_Up,    ImGuiKey_UpArrow);
        keyEvent(HidNpadButton_Down,  ImGuiKey_DownArrow);
        keyEvent(HidNpadButton_Left,  ImGuiKey_LeftArrow);
        keyEvent(HidNpadButton_Right, ImGuiKey_RightArrow);
    }

    HidAnalogStickState leftStick = padGetStickPos(pad, 0);
    const float threshold = 20000.0f;

    static bool stickUpPrev    = false;
    static bool stickDownPrev  = false;
    static bool stickLeftPrev  = false;
    static bool stickRightPrev = false;

    bool stickUp    = leftStick.y >  threshold;
    bool stickDown  = leftStick.y < -threshold;
    bool stickLeft  = leftStick.x < -threshold;
    bool stickRight = leftStick.x >  threshold;

    if (enableNav) {
        if (stickUp    && !stickUpPrev)    io.AddKeyEvent(ImGuiKey_UpArrow,    true);
        if (!stickUp   && stickUpPrev)     io.AddKeyEvent(ImGuiKey_UpArrow,    false);
        if (stickDown  && !stickDownPrev)  io.AddKeyEvent(ImGuiKey_DownArrow,  true);
        if (!stickDown && stickDownPrev)   io.AddKeyEvent(ImGuiKey_DownArrow,  false);
        if (stickLeft  && !stickLeftPrev)  io.AddKeyEvent(ImGuiKey_LeftArrow,  true);
        if (!stickLeft && stickLeftPrev)   io.AddKeyEvent(ImGuiKey_LeftArrow,  false);
        if (stickRight && !stickRightPrev) io.AddKeyEvent(ImGuiKey_RightArrow, true);
        if (!stickRight && stickRightPrev) io.AddKeyEvent(ImGuiKey_RightArrow, false);
    }

    stickUpPrev    = stickUp;
    stickDownPrev  = stickDown;
    stickLeftPrev  = stickLeft;
    stickRightPrev = stickRight;
}

void ImGuiProcessSDLEvents(void) {
    ImGuiIO& io = ImGui::GetIO();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_FINGERDOWN:
                io.AddMousePosEvent(e.tfinger.x * 1280.0f, e.tfinger.y * 720.0f);
                io.AddMouseButtonEvent(0, true);
                break;
            case SDL_FINGERUP:
                io.AddMousePosEvent(e.tfinger.x * 1280.0f, e.tfinger.y * 720.0f);
                io.AddMouseButtonEvent(0, false);
                break;
            case SDL_FINGERMOTION:
                io.AddMousePosEvent(e.tfinger.x * 1280.0f, e.tfinger.y * 720.0f);
                break;
            case SDL_MOUSEMOTION:
                io.AddMousePosEvent((float)e.motion.x, (float)e.motion.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    io.AddMousePosEvent((float)e.button.x, (float)e.button.y);
                    io.AddMouseButtonEvent(0, true);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    io.AddMousePosEvent((float)e.button.x, (float)e.button.y);
                    io.AddMouseButtonEvent(0, false);
                }
                break;
            default:
                break;
        }
    }
}
