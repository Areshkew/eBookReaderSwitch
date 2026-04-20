extern "C" {
    #include "MenuChooser.h"
    #include "menu_book_reader.h"
    #include "common.h"
    #include "fs.h"
    #include "config.h"
    #include "paths.h"
    #include "logger.h"
}

#include <switch.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <cstring>

#include <SDL2/SDL.h>

#include "imgui.h"
#include "imgui_impl_sdlrenderer2.h"

using namespace std;
namespace fs = filesystem;

struct BookEntry {
    string filename;
    string path;
    string ext;
    bool isExperimental;
};

static void SetSwitchTheme(bool dark) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

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

static void UpdateImGuiInput(PadState& pad) {
    ImGuiIO& io = ImGui::GetIO();
    u64 kDown = padGetButtonsDown(&pad);
    u64 kUp   = padGetButtonsUp(&pad);

    auto keyEvent = [&](HidNpadButton btn, ImGuiKey key) {
        if (kDown & btn) io.AddKeyEvent(key, true);
        if (kUp   & btn) io.AddKeyEvent(key, false);
    };

    // Map Switch buttons to keyboard keys for ImGui navigation
    // (Using keyboard nav instead of gamepad nav avoids requiring
    // a platform backend that sets ImGuiBackendFlags_HasGamepad.)
    keyEvent(HidNpadButton_A,     ImGuiKey_Enter);
    keyEvent(HidNpadButton_B,     ImGuiKey_Escape);
    keyEvent(HidNpadButton_Up,    ImGuiKey_UpArrow);
    keyEvent(HidNpadButton_Down,  ImGuiKey_DownArrow);
    keyEvent(HidNpadButton_Left,  ImGuiKey_LeftArrow);
    keyEvent(HidNpadButton_Right, ImGuiKey_RightArrow);
    keyEvent(HidNpadButton_Plus,  ImGuiKey_Space);
    keyEvent(HidNpadButton_Minus, ImGuiKey_Tab);

    // Analog sticks → keyboard arrows (one-shot, no analog repeat)
    HidAnalogStickState leftStick = padGetStickPos(&pad, 0);
    const float threshold = 20000.0f;

    static bool stickUpPrev    = false;
    static bool stickDownPrev  = false;
    static bool stickLeftPrev  = false;
    static bool stickRightPrev = false;

    bool stickUp    = leftStick.y >  threshold;
    bool stickDown  = leftStick.y < -threshold;
    bool stickLeft  = leftStick.x < -threshold;
    bool stickRight = leftStick.x >  threshold;

    if (stickUp    && !stickUpPrev)    io.AddKeyEvent(ImGuiKey_UpArrow,    true);
    if (!stickUp   && stickUpPrev)     io.AddKeyEvent(ImGuiKey_UpArrow,    false);
    if (stickDown  && !stickDownPrev)  io.AddKeyEvent(ImGuiKey_DownArrow,  true);
    if (!stickDown && stickDownPrev)   io.AddKeyEvent(ImGuiKey_DownArrow,  false);
    if (stickLeft  && !stickLeftPrev)  io.AddKeyEvent(ImGuiKey_LeftArrow,  true);
    if (!stickLeft && stickLeftPrev)   io.AddKeyEvent(ImGuiKey_LeftArrow,  false);
    if (stickRight && !stickRightPrev) io.AddKeyEvent(ImGuiKey_RightArrow, true);
    if (!stickRight && stickRightPrev) io.AddKeyEvent(ImGuiKey_RightArrow, false);

    stickUpPrev    = stickUp;
    stickDownPrev  = stickDown;
    stickLeftPrev  = stickLeft;
    stickRightPrev = stickRight;
}

void Menu_StartChoosing() {
    vector<BookEntry> books;
    list<string> allowedExtensions = {".pdf", ".epub", ".cbz", ".xps"};
    list<string> warnedExtensions  = {".epub", ".cbz", ".xps"};

    string booksDir = BOOKS_DIR;

    if (FS_DirExists(booksDir.c_str())) {
        for (const auto& entry : fs::directory_iterator(booksDir)) {
            string filename = entry.path().filename().string();
            size_t dotPos = filename.find_last_of('.');
            if (dotPos == string::npos)
                continue;

            string ext = filename.substr(dotPos);

            if (find(allowedExtensions.begin(), allowedExtensions.end(), ext) != allowedExtensions.end()) {
                BookEntry book;
                book.filename = filename;
                book.path = entry.path().string();
                book.ext = ext;
                book.isExperimental = find(warnedExtensions.begin(), warnedExtensions.end(), ext) != warnedExtensions.end();
                books.push_back(book);
            }
        }
    }

    sort(books.begin(), books.end(), [](const BookEntry& a, const BookEntry& b) {
        return a.filename < b.filename;
    });

    PadState pad;
    padInitializeDefault(&pad);

    bool showWarningModal = false;
    int warningBookIdx = -1;

    const char* extItems[] = {"All", "PDF", "EPUB", "CBZ", "XPS"};
    int extFilter = 0;
    int prevExtFilter = 0;

    vector<int> filteredIndices;
    for (int i = 0; i < (int)books.size(); i++)
        filteredIndices.push_back(i);

    string bookToOpen;
    bool openBook = false;

    while (appletMainLoop()) {
        // Pump SDL events
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            (void)e;
        }

        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        // App-level controls
        if (!showWarningModal && (kDown & HidNpadButton_Plus))
            break;
        if (kDown & HidNpadButton_Minus)
            configDarkMode = !configDarkMode;
        if (!showWarningModal && (kDown & HidNpadButton_B))
            break;

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();

        // Hardcode display parameters for Switch (workaround SDL2 reporting issues)
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        if (io.DeltaTime <= 0.0f || io.DeltaTime > 1.0f)
            io.DeltaTime = 1.0f / 60.0f;

        ImGui::NewFrame();

        UpdateImGuiInput(pad);
        SetSwitchTheme(configDarkMode);

        // ---- Main fullscreen window ----
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1280, 720));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
        ImGui::Begin("BookChooser", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus);
        ImGui::PopStyleVar();

        // ---- Header ----
        ImVec4 accentColor = configDarkMode ? ImVec4(0.00f, 0.82f, 1.00f, 1.00f) : ImVec4(0.00f, 0.55f, 0.75f, 1.00f);
        ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
        ImGui::Text("eBook Reader");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 200);
        if (ImGui::Button(configDarkMode ? "Light Theme" : "Dark Theme", ImVec2(180, 40))) {
            configDarkMode = !configDarkMode;
        }

        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        // ---- Stats bar ----
        ImGui::Text("Books: %d", (int)books.size());
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 350);
        ImGui::TextDisabled("Library: %s", booksDir.c_str());

        ImGui::Dummy(ImVec2(0, 8));

        // ---- Filters ----
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        if (ImGui::BeginCombo("##extfilter", extItems[extFilter])) {
            for (int n = 0; n < IM_ARRAYSIZE(extItems); n++) {
                bool isSelected = (extFilter == n);
                if (ImGui::Selectable(extItems[n], isSelected))
                    extFilter = n;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Rebuild filtered list if extension filter changed
        if (extFilter != prevExtFilter) {
            prevExtFilter = extFilter;
            filteredIndices.clear();

            for (int i = 0; i < (int)books.size(); i++) {
                const BookEntry& book = books[i];

                // Extension filter
                if (extFilter > 0) {
                    string targetExt = string(".") + extItems[extFilter];
                    transform(targetExt.begin(), targetExt.end(), targetExt.begin(), ::tolower);
                    string bookExtLower = book.ext;
                    transform(bookExtLower.begin(), bookExtLower.end(), bookExtLower.begin(), ::tolower);
                    if (bookExtLower != targetExt)
                        continue;
                }

                filteredIndices.push_back(i);
            }
        }

        ImGui::Dummy(ImVec2(0, 8));

        // ---- Book list ----
        ImGui::BeginChild("BookList", ImVec2(0, -60), ImGuiChildFlags_NavFlattened);

        for (int i = 0; i < (int)filteredIndices.size(); i++) {
            int bookIdx = filteredIndices[i];
            const BookEntry& book = books[bookIdx];

            ImGui::PushID(bookIdx);

            // Invisible selectable handles focus + activation
            bool activated = ImGui::Selectable("##hidden", false, 0, ImVec2(0, 48));

            // Custom drawing on top of the selectable
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            float textY = min.y + (48.0f - ImGui::GetFontSize()) * 0.5f;
            float x = min.x + 14.0f;

            // Warning indicator bar for experimental formats
            if (book.isExperimental) {
                drawList->AddRectFilled(
                    ImVec2(min.x + 4, min.y + 10),
                    ImVec2(min.x + 8, max.y - 10),
                    IM_COL32(255, 180, 0, 255), 2.0f);
                x += 12.0f;
            }

            // Filename
            drawList->AddText(ImVec2(x, textY), ImGui::GetColorU32(ImGuiCol_Text), book.filename.c_str());

            // Extension badge on the right
            float badgeTextWidth = ImGui::CalcTextSize(book.ext.c_str()).x;
            float badgeWidth = badgeTextWidth + 14.0f;
            float badgeX = max.x - badgeWidth - 14.0f;
            ImU32 badgeBg = configDarkMode ? IM_COL32(60, 60, 60, 200) : IM_COL32(200, 200, 200, 200);
            ImU32 badgeText = configDarkMode ? IM_COL32(160, 160, 160, 255) : IM_COL32(80, 80, 80, 255);
            drawList->AddRectFilled(
                ImVec2(badgeX, textY - 2),
                ImVec2(badgeX + badgeWidth, textY + ImGui::GetFontSize() + 2),
                badgeBg, 4.0f);
            drawList->AddText(
                ImVec2(badgeX + 7, textY), badgeText, book.ext.c_str());

            if (activated) {
                if (book.isExperimental) {
                    warningBookIdx = bookIdx;
                    showWarningModal = true;
                } else {
                    bookToOpen = book.path;
                    openBook = true;
                }
            }

            ImGui::PopID();
        }

        if (filteredIndices.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::TextWrapped("No books found.\n\nPlace .pdf, .epub, .cbz, or .xps files in:\n%s", booksDir.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::EndChild();

        // ---- Footer hints ----
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::TextDisabled("[A] Open Book    [B] Back    [+] Exit    [-] Toggle Theme");

        ImGui::End();

        // ---- Warning modal ----
        if (showWarningModal) {
            ImGui::OpenPopup("Warning");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(600, 220));

        if (ImGui::BeginPopupModal("Warning", &showWarningModal,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
        {
            ImGui::Text("This file format is not fully supported.");
            ImGui::Text("It may cause the system or app to crash.");
            ImGui::Dummy(ImVec2(0, 15));

            float buttonWidth = 160.0f;
            float windowWidth = ImGui::GetWindowSize().x;
            ImGui::SetCursorPosX((windowWidth - buttonWidth * 2.0f - 20.0f) * 0.5f);

            if (ImGui::Button("Read Anyway", ImVec2(buttonWidth, 40))) {
                ImGui::CloseCurrentPopup();
                showWarningModal = false;
                if (warningBookIdx >= 0) {
                    bookToOpen = books[warningBookIdx].path;
                    openBook = true;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(buttonWidth, 40))) {
                ImGui::CloseCurrentPopup();
                showWarningModal = false;
                warningBookIdx = -1;
            }

            ImGui::EndPopup();
        }

        // ---- Render ----
        ImGui::Render();
        SDL_SetRenderDrawColor(RENDERER, 0, 0, 0, 255);
        SDL_RenderClear(RENDERER);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), RENDERER);
        SDL_RenderPresent(RENDERER);

        // ---- Open book if requested ----
        if (openBook) {
            openBook = false;
            LOG_I("Opening book: %s", bookToOpen.c_str());
            Menu_OpenBook((char*)bookToOpen.c_str());
            break; // exit chooser after reader returns (matches original behavior)
        }
    }

}
