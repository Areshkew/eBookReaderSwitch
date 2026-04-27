extern "C" {
    #include "fs.h"
    #include "paths.h"
    #include "logger.h"
}

#include "app.h"
#include "menu_chooser.h"
#include "menu_book_reader.h"
#include "theme.h"

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
#include "imgui_switch.h"

using namespace std;
namespace fs = filesystem;

struct BookEntry {
    string filename;
    string path;
    string ext;
    bool isExperimental;
};

void Menu_StartChoosing(App& app) {
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
    bool showThemeModal = false;

    const char* extItems[] = {"All", "PDF", "EPUB", "CBZ", "XPS"};
    int extFilter = 0;
    int prevExtFilter = 0;

    vector<int> filteredIndices;
    for (int i = 0; i < (int)books.size(); i++)
        filteredIndices.push_back(i);

    string bookToOpen;
    bool openBook = false;

    while (appletMainLoop()) {
        ImGuiProcessSDLEvents();

        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        u64 kUp   = padGetButtonsUp(&pad);

        if (!showWarningModal && !showThemeModal && (kUp & HidNpadButton_Plus))
            app.setNightMode(!app.nightMode());
        if (!showWarningModal && !showThemeModal && (kDown & HidNpadButton_Minus))
            showThemeModal = true;
        if (!showWarningModal && !showThemeModal && (kDown & HidNpadButton_B))
            break;

        ImGui_ImplSDLRenderer2_NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        if (io.DeltaTime <= 0.0f || io.DeltaTime > 1.0f)
            io.DeltaTime = 1.0f / 60.0f;

        ImGui::NewFrame();

        ImGuiUpdateSwitchInput(&pad);
        ImGuiSetSwitchTheme(app.darkMode());

        const Theme& th = app.theme();
        ImVec4 accentColor(th.accent.r / 255.0f, th.accent.g / 255.0f, th.accent.b / 255.0f, th.accent.a / 255.0f);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(1280, 720));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 24));
        ImGui::Begin("BookChooser", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus);
        ImGui::PopStyleVar();

        ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
        ImGui::Text("Switchelf");
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 340);
        if (ImGui::Button("Theme", ImVec2(130, 40))) {
            showThemeModal = true;
        }

        ImGui::SameLine();
        if (ImGui::Button(app.nightMode() ? "Night: On" : "Night: Off", ImVec2(150, 40))) {
            app.setNightMode(!app.nightMode());
        }

        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 8));

        ImGui::Text("Books: %d", (int)books.size());
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 300);
        ImGui::TextDisabled("Library: %s", booksDir.c_str());

        ImGui::Dummy(ImVec2(0, 8));

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

        if (extFilter != prevExtFilter) {
            prevExtFilter = extFilter;
            filteredIndices.clear();

            for (int i = 0; i < (int)books.size(); i++) {
                const BookEntry& book = books[i];

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

        ImGui::BeginChild("BookList", ImVec2(0, -60), ImGuiChildFlags_NavFlattened);

        for (int i = 0; i < (int)filteredIndices.size(); i++) {
            int bookIdx = filteredIndices[i];
            const BookEntry& book = books[bookIdx];

            ImGui::PushID(bookIdx);

            bool activated = ImGui::Selectable("##hidden", false, 0, ImVec2(0, 48));

            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            float textY = min.y + (48.0f - ImGui::GetFontSize()) * 0.5f;
            float x = min.x + 14.0f;

            if (book.isExperimental) {
                drawList->AddRectFilled(
                    ImVec2(min.x + 4, min.y + 10),
                    ImVec2(min.x + 8, max.y - 10),
                    IM_COL32(255, 180, 0, 255), 2.0f);
                x += 12.0f;
            }

            drawList->AddText(ImVec2(x, textY), ImGui::GetColorU32(ImGuiCol_Text), book.filename.c_str());

            float badgeTextWidth = ImGui::CalcTextSize(book.ext.c_str()).x;
            float badgeWidth = badgeTextWidth + 14.0f;
            float badgeX = max.x - badgeWidth - 14.0f;
            ImU32 badgeBg = IM_COL32(th.badge_bg.r, th.badge_bg.g, th.badge_bg.b, th.badge_bg.a);
            ImU32 badgeText = IM_COL32(th.badge_text.r, th.badge_text.g, th.badge_text.b, th.badge_text.a);
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

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::TextDisabled("[A] Open Book    [B] Back    [+] Night");

        ImGui::End();

        if (showWarningModal) {
            ImGui::OpenPopup("Warning");
        }

        if (showThemeModal) {
            ImGui::OpenPopup("Themes");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(500, 450));

        if (ImGui::BeginPopupModal("Themes", &showThemeModal,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
        {
            auto themeList = app.themes->list_themes();
            const string& active = app.config.settings().active_theme;

            ImGui::Text("Select Theme");
            ImGui::Separator();
            ImGui::Dummy(ImVec2(0, 8));

            ImGui::BeginChild("ThemeList", ImVec2(0, -50), ImGuiChildFlags_NavFlattened);
            for (const auto& tname : themeList) {
                bool isSelected = (tname == active);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Text, accentColor);
                }
                if (ImGui::Selectable(tname.c_str(), isSelected, 0, ImVec2(0, 36))) {
                    if (tname != active) {
                        app.themes->load_theme(tname.c_str());
                        app.config.mutable_settings().active_theme = tname;
                        app.config.mark_dirty();
                        app.config.save();
                    }
                    showThemeModal = false;
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
                showThemeModal = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

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

        ImGui::Render();
        SDL_SetRenderDrawColor(app.renderer, th.background.r, th.background.g, th.background.b, 255);
        SDL_RenderClear(app.renderer);
        if (app.nightMode()) {
            SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(app.renderer, th.night_overlay.r, th.night_overlay.g, th.night_overlay.b, th.night_overlay.alpha);
            SDL_RenderFillRect(app.renderer, nullptr);
        }
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), app.renderer);
        SDL_RenderPresent(app.renderer);

        if (openBook) {
            openBook = false;
            LOG_I("Opening book: %s", bookToOpen.c_str());
            Menu_OpenBook(app, bookToOpen.c_str());
            break;
        }
    }
}
