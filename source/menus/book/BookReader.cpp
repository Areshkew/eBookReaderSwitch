#include "BookReader.hpp"
#include "PageLayout.hpp"
#include "LandscapePageLayout.hpp"
#include <algorithm>
#include <libconfig.h>
#include <cstdio>

extern "C"  {
    #include "SDL_helper.h"
    #include "config.h"
    #include "common.h"
    #include "paths.h"
    #include "logger.h"
}

#include "imgui.h"

fz_context *ctx = NULL;
int windowX, windowY;
config_t *config = NULL;
char* configFile = CONFIG_FILE;

static int load_last_page(const char *book_name)  {
    if (!config) {
        config = (config_t *)malloc(sizeof(config_t));
        config_init(config);
        if (!config_read_file(config, configFile)) {
            LOG_W("Failed to read config file: %s", configFile);
        }
    }
    
    config_setting_t *setting = config_setting_get_member(config_root_setting(config), book_name);
    
    if (setting) {
        return config_setting_get_int(setting);
    }

    return 0;
}

static void save_last_page(const char *book_name, int current_page) {
    config_setting_t *setting = config_setting_get_member(config_root_setting(config), book_name);
    
    if (!setting) {
        setting = config_setting_add(config_root_setting(config), book_name, CONFIG_TYPE_INT);
    }
    
    if (setting) {
        config_setting_set_int(setting, current_page);
        if (!config_write_file(config, configFile)) {
            LOG_W("Failed to write config file: %s", configFile);
        }
    }
}

BookReader::BookReader(const char *path, int* result) {
    if (ctx == NULL) {
        ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
        fz_register_document_handlers(ctx);
    }

    SDL_GetWindowSize(WINDOW, &windowX, &windowY);
    
    book_name = std::string(path).substr(std::string(path).find_last_of("/\\") + 1);
    
    std::string invalid_chars = " :/?#[]@!$&'()*+,;=.";
    for (char& c: invalid_chars) {
        book_name.erase(std::remove(book_name.begin(), book_name.end(), c), book_name.end());
    }
    
    fz_try(ctx)	{
        LOG_I("fz_open_document");
        doc = fz_open_document(ctx, path);

        if (!doc)
        {
            LOG_E("Error opening file!");
            *result = -1;
            return;
        }
        
        int current_page = load_last_page(book_name.c_str());

        LOG_I("current_page = %d", current_page);

        switch_current_page_layout(_currentPageLayout, current_page);
    }
    fz_catch(ctx){
        LOG_E("fz_catch reached, closing gracefully");
        *result = -2;
        return;
    }
}

BookReader::~BookReader() {
    fz_drop_document(ctx, doc);
    
    delete layout;
}

void BookReader::previous_page(int n) {
    layout->previous_page(n);
    save_last_page(book_name.c_str(), layout->current_page());
}

void BookReader::next_page(int n) {
    layout->next_page(n);
    save_last_page(book_name.c_str(), layout->current_page());
}

void BookReader::zoom_in() {
    layout->zoom_in();
}

void BookReader::zoom_out() {
    layout->zoom_out();
}

void BookReader::move_page_up() {
    layout->move_up();
}

void BookReader::move_page_down() {
    layout->move_down();
}

void BookReader::move_page_left() {
    layout->move_left();
}

void BookReader::move_page_right() {
    layout->move_right();
}

void BookReader::pan_page(float dx, float dy) {
    layout->pan(dx, dy);
}

void BookReader::zoom_by(float factor) {
    layout->zoom_by(factor);
}

void BookReader::reset_page() {
    layout->reset();
}

void BookReader::switch_page_layout() {
    switch (_currentPageLayout) {
        case BookPageLayoutPortrait:
            switch_current_page_layout(BookPageLayoutLandscape, 0);
            break;
        case BookPageLayoutLandscape:
            switch_current_page_layout(BookPageLayoutPortrait, 0);
            break;
    }
}

void BookReader::draw() {
    if (configDarkMode == true) {
        SDL_ClearScreen(RENDERER, BLACK);
    } else {
        SDL_ClearScreen(RENDERER, WHITE);
    }

    SDL_RenderClear(RENDERER);
    
    layout->draw_page();

    if (showUI) {
        ImGuiIO& io = ImGui::GetIO();
        float displayW = io.DisplaySize.x;
        float displayH = io.DisplaySize.y;

        bool isPortrait = (_currentPageLayout == BookPageLayoutPortrait);

        // Bar dimensions
        const float topBarH = 55.0f;
        const float bottomBarH = 70.0f;
        const float landBarW = 90.0f;

        // Distinct bar colours so they’re visible against the page
        ImVec4 barBg = configDarkMode
            ? ImVec4(0.14f, 0.14f, 0.14f, 1.00f)
            : ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
        ImVec4 sepCol = configDarkMode
            ? ImVec4(0.30f, 0.30f, 0.30f, 1.00f)
            : ImVec4(0.60f, 0.60f, 0.60f, 1.00f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, barBg);

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        ImVec4 activeBtnCol = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];

        if (isPortrait) {
            // ========== PORTRAIT: horizontal bars ==========

            // --- Top Bar ---
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(displayW, topBarH));
            ImGui::Begin("TopBar", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoNavFocus);

            if (ImGui::Button("<", ImVec2(36, 34))) {
                requestExit = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Back to library");
            }

            ImGui::SameLine();
            float availW = ImGui::GetContentRegionAvail().x;
            float titleWidth = ImGui::CalcTextSize(book_name.c_str()).x;
            float titleX = (availW - titleWidth) * 0.5f;
            if (titleX < 0) titleX = 0;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + titleX);
            ImGui::Text("%s", book_name.c_str());

            ImGui::SameLine();
            ImGui::SetCursorPosX(displayW - 240);
            if (ImGui::Button("Aa", ImVec2(36, 34))) { /* placeholder: text settings */ }
            ImGui::SameLine();
            if (ImGui::Button("::", ImVec2(36, 34))) { /* placeholder: TOC */ }
            ImGui::SameLine();
            if (ImGui::Button("Bm", ImVec2(36, 34))) { /* placeholder: bookmark */ }
            ImGui::SameLine();
            if (ImGui::Button("Q", ImVec2(36, 34))) { /* placeholder: search */ }
            ImGui::SameLine();
            if (ImGui::Button(":", ImVec2(36, 34))) { /* placeholder: more */ }

            ImGui::End();
            drawList->AddLine(ImVec2(0, topBarH), ImVec2(displayW, topBarH),
                ImGui::ColorConvertFloat4ToU32(sepCol), 1.0f);

            // --- Bottom Bar ---
            float bottomY = displayH - bottomBarH;
            ImGui::SetNextWindowPos(ImVec2(0, bottomY));
            ImGui::SetNextWindowSize(ImVec2(displayW, bottomBarH));
            ImGui::Begin("BottomBar", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoNavFocus);

            int cur = layout->current_page() + 1;
            int tot = layout->total_pages();
            int pct = tot > 0 ? (cur * 100 / tot) : 0;
            char infoBuf[128];
            snprintf(infoBuf, sizeof(infoBuf), "Page %d of %d  |  %d%%", cur, tot, pct);
            float infoWidth = ImGui::CalcTextSize(infoBuf).x;
            ImGui::SetCursorPosX((displayW - infoWidth) * 0.5f);
            ImGui::Text("%s", infoBuf);

            float btnW = 90.0f;
            float gap = 16.0f;
            float groupW = btnW * 2 + gap;
            ImGui::SetCursorPosX((displayW - groupW) * 0.5f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);

            if (_currentPageLayout == BookPageLayoutPortrait) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeBtnCol);
            }
            if (ImGui::Button("[|]", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutPortrait)
                    switch_page_layout();
            }
            if (_currentPageLayout == BookPageLayoutPortrait) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Portrait layout");
            }

            ImGui::SameLine(0, gap);
            if (_currentPageLayout == BookPageLayoutLandscape) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeBtnCol);
            }
            if (ImGui::Button("[| |]", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutLandscape)
                    switch_page_layout();
            }
            if (_currentPageLayout == BookPageLayoutLandscape) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Landscape layout");
            }

            ImGui::End();
            drawList->AddLine(ImVec2(0, bottomY), ImVec2(displayW, bottomY),
                ImGui::ColorConvertFloat4ToU32(sepCol), 1.0f);

        } else {
            // ========== LANDSCAPE: vertical bars ==========
            // Page is rotated 90° clockwise, so:
            //   page top    -> screen right edge
            //   page bottom -> screen left edge
            // Right edge bar = former top bar (back, title, icons)
            // Left edge bar  = former bottom bar (page info, layout)

            const float landBarW = 110.0f;
            auto drawTextBottomToTop = [&](const std::string& text, float cx, float startY, float maxH) {
                float y = startY;
                float charH = ImGui::GetTextLineHeightWithSpacing();
                float totalH = text.length() * charH;
                if (totalH > maxH) {
                    totalH = maxH;
                }
                // Clamp startY so text fits within maxH upward
                float bottomY = startY;
                if (bottomY - totalH < 10.0f) {
                    bottomY = 10.0f + totalH;
                }
                y = bottomY;
                for (size_t i = 0; i < text.length() && (bottomY - y) < maxH; ++i) {
                    char buf[2] = {text[i], '\0'};
                    ImVec2 ts = ImGui::CalcTextSize(buf);
                    ImGui::SetCursorPosX(cx + (landBarW - 28 - ts.x) * 0.5f);
                    ImGui::SetCursorPosY(y - ts.y);
                    ImGui::Text("%s", buf);
                    y -= ts.y;
                }
            };

            // --- Right Bar (top content) ---
            float rightX = displayW - landBarW;
            ImGui::SetNextWindowPos(ImVec2(rightX, 0));
            ImGui::SetNextWindowSize(ImVec2(landBarW, displayH));
            ImGui::Begin("RightBar", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoNavFocus);

            if (ImGui::Button("<", ImVec2(82, 34))) {
                requestExit = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Back to library");
            }

            // Title written vertically bottom-to-top so it reads naturally
            // when the device is held in landscape (tilt left to read)
            std::string displayTitle = book_name;
            if (displayTitle.length() > 40) {
                displayTitle = displayTitle.substr(0, 37) + "...";
            }
            float cxBase = ImGui::GetCursorPosX();
            drawTextBottomToTop(displayTitle, cxBase, displayH - 210, displayH - 300);

            // Icon buttons at the bottom of the right bar
            float by = displayH - 190;
            ImGui::SetCursorPosY(by);
            if (ImGui::Button("Aa", ImVec2(82, 34))) { /* placeholder: text settings */ }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("::", ImVec2(82, 34))) { /* placeholder: TOC */ }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("Bm", ImVec2(82, 34))) { /* placeholder: bookmark */ }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("Q", ImVec2(82, 34))) { /* placeholder: search */ }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button(":", ImVec2(82, 34))) { /* placeholder: more */ }

            ImGui::End();
            drawList->AddLine(ImVec2(rightX, 0), ImVec2(rightX, displayH),
                ImGui::ColorConvertFloat4ToU32(sepCol), 1.0f);

            // --- Left Bar (bottom content) ---
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(landBarW, displayH));
            ImGui::Begin("LeftBar", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoNavFocus);

            // Layout toggle buttons at the top of the left bar
            float btnW = 82.0f;
            if (_currentPageLayout == BookPageLayoutPortrait) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeBtnCol);
            }
            if (ImGui::Button("[|]", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutPortrait)
                    switch_page_layout();
            }
            if (_currentPageLayout == BookPageLayoutPortrait) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Portrait layout");
            }

            ImGui::Dummy(ImVec2(0, 6));
            if (_currentPageLayout == BookPageLayoutLandscape) {
                ImGui::PushStyleColor(ImGuiCol_Button, activeBtnCol);
            }
            if (ImGui::Button("[| |]", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutLandscape)
                    switch_page_layout();
            }
            if (_currentPageLayout == BookPageLayoutLandscape) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Landscape layout");
            }

            // Page info drawn bottom-to-top at the bottom of the left bar
            int cur = layout->current_page() + 1;
            int tot = layout->total_pages();
            int pct = tot > 0 ? (cur * 100 / tot) : 0;
            char infoBuf[64];
            snprintf(infoBuf, sizeof(infoBuf), "Pg %d/%d", cur, tot);
            std::string infoLine1 = infoBuf;
            snprintf(infoBuf, sizeof(infoBuf), "%d%%", pct);
            std::string infoLine2 = infoBuf;
            std::string infoText = infoLine1 + " " + infoLine2;

            float cxBaseL = ImGui::GetCursorPosX();
            drawTextBottomToTop(infoText, cxBaseL, displayH - 20, 120);

            ImGui::End();
            drawList->AddLine(ImVec2(landBarW, 0), ImVec2(landBarW, displayH),
                ImGui::ColorConvertFloat4ToU32(sepCol), 1.0f);
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    }
}

void BookReader::switch_current_page_layout(BookPageLayout bookPageLayout, int current_page) {
    if (layout) {
        current_page = layout->current_page();
        delete layout;
        layout = NULL;
    }
    
    _currentPageLayout = bookPageLayout;
    
    switch (bookPageLayout) {
        case BookPageLayoutPortrait:
            layout = new PageLayout(doc, current_page);
            break;
        case BookPageLayoutLandscape:
            layout = new LandscapePageLayout(doc, current_page);
            break;
    }
}
