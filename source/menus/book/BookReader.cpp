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
#include "imgui_internal.h"

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

static void RotateDrawListVertices(ImDrawList* draw_list, int vtx_start, float cos_a, float sin_a, const ImVec2& center) {
    auto& buf = draw_list->VtxBuffer;
    for (int i = vtx_start; i < buf.Size; ++i) {
        ImVec2 v(buf[i].pos.x - center.x, buf[i].pos.y - center.y);
        ImVec2 r = ImRotate(v, cos_a, sin_a);
        buf[i].pos.x = r.x + center.x;
        buf[i].pos.y = r.y + center.y;
    }
}

static void DrawRotatedIcon(ImDrawList* draw_list, ImTextureID tex_id, const ImVec2& pos, const ImVec2& size, float cos_a, float sin_a, ImU32 col = IM_COL32_WHITE) {
    ImVec2 center(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    ImVec2 corners[4] = {
        ImVec2(pos.x,         pos.y),
        ImVec2(pos.x + size.x, pos.y),
        ImVec2(pos.x + size.x, pos.y + size.y),
        ImVec2(pos.x,         pos.y + size.y)
    };
    for (int i = 0; i < 4; ++i) {
        ImVec2 v(corners[i].x - center.x, corners[i].y - center.y);
        ImVec2 r = ImRotate(v, cos_a, sin_a);
        corners[i].x = r.x + center.x;
        corners[i].y = r.y + center.y;
    }
    draw_list->AddImageQuad(tex_id, corners[0], corners[1], corners[2], corners[3],
                            ImVec2(0,0), ImVec2(1,0), ImVec2(1,1), ImVec2(0,1), col);
}

static ImVec2 CenteredIconPos(const ImRect& bb, const ImVec2& iconSize) {
    return ImVec2(bb.Min.x + (bb.GetWidth()  - iconSize.x) * 0.5f,
                  bb.Min.y + (bb.GetHeight() - iconSize.y) * 0.5f);
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

        load_icons();
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
    free_icons();
}

void BookReader::load_icons() {
    SDL_LoadImage(&icon_back,      (char*)"romfs:/resources/icons/icon_back.png");
    SDL_LoadImage(&icon_text,      (char*)"romfs:/resources/icons/icon_text.png");
    SDL_LoadImage(&icon_toc,       (char*)"romfs:/resources/icons/icon_toc.png");
    SDL_LoadImage(&icon_bookmark,  (char*)"romfs:/resources/icons/icon_bookmark.png");
    SDL_LoadImage(&icon_search,    (char*)"romfs:/resources/icons/icon_search.png");
    SDL_LoadImage(&icon_more,      (char*)"romfs:/resources/icons/icon_more.png");
    SDL_LoadImage(&icon_portrait,  (char*)"romfs:/resources/icons/icon_portrait.png");
    SDL_LoadImage(&icon_landscape, (char*)"romfs:/resources/icons/icon_landscape.png");
}

void BookReader::free_icons() {
    if (icon_back)      { SDL_DestroyTexture(icon_back);      icon_back      = nullptr; }
    if (icon_text)      { SDL_DestroyTexture(icon_text);      icon_text      = nullptr; }
    if (icon_toc)       { SDL_DestroyTexture(icon_toc);       icon_toc       = nullptr; }
    if (icon_bookmark)  { SDL_DestroyTexture(icon_bookmark);  icon_bookmark  = nullptr; }
    if (icon_search)    { SDL_DestroyTexture(icon_search);    icon_search    = nullptr; }
    if (icon_more)      { SDL_DestroyTexture(icon_more);      icon_more      = nullptr; }
    if (icon_portrait)  { SDL_DestroyTexture(icon_portrait);  icon_portrait  = nullptr; }
    if (icon_landscape) { SDL_DestroyTexture(icon_landscape); icon_landscape = nullptr; }
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

            if (ImGui::Button("##BackP", ImVec2(36, 34))) {
                requestExit = true;
            }
            if (icon_back) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 28) * 0.5f, min.y + (max.y - min.y - 28) * 0.5f);
                drawList->AddImage((ImTextureID)icon_back, pos, ImVec2(pos.x + 28, pos.y + 28));
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
            if (ImGui::Button("##TextP", ImVec2(36, 34))) { /* placeholder: text settings */ }
            if (icon_text) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 28) * 0.5f, min.y + (max.y - min.y - 28) * 0.5f);
                drawList->AddImage((ImTextureID)icon_text, pos, ImVec2(pos.x + 28, pos.y + 28));
            }
            ImGui::SameLine();
            if (ImGui::Button("##TOCP", ImVec2(36, 34))) { /* placeholder: TOC */ }
            if (icon_toc) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 28) * 0.5f, min.y + (max.y - min.y - 28) * 0.5f);
                drawList->AddImage((ImTextureID)icon_toc, pos, ImVec2(pos.x + 28, pos.y + 28));
            }
            ImGui::SameLine();
            if (ImGui::Button("##BookmarkP", ImVec2(36, 34))) { /* placeholder: bookmark */ }
            if (icon_bookmark) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 28) * 0.5f, min.y + (max.y - min.y - 28) * 0.5f);
                drawList->AddImage((ImTextureID)icon_bookmark, pos, ImVec2(pos.x + 28, pos.y + 28));
            }
            ImGui::SameLine();
            if (ImGui::Button("##SearchP", ImVec2(36, 34))) { /* placeholder: search */ }
            if (icon_search) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 28) * 0.5f, min.y + (max.y - min.y - 28) * 0.5f);
                drawList->AddImage((ImTextureID)icon_search, pos, ImVec2(pos.x + 28, pos.y + 28));
            }
            ImGui::SameLine();
            if (ImGui::Button("##MoreP", ImVec2(36, 34))) { /* placeholder: more */ }
            if (icon_more) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 28) * 0.5f, min.y + (max.y - min.y - 28) * 0.5f);
                drawList->AddImage((ImTextureID)icon_more, pos, ImVec2(pos.x + 28, pos.y + 28));
            }

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
            if (ImGui::Button("##PortraitP", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutPortrait)
                    switch_page_layout();
            }
            if (icon_portrait) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 24) * 0.5f, min.y + (max.y - min.y - 24) * 0.5f);
                drawList->AddImage((ImTextureID)icon_portrait, pos, ImVec2(pos.x + 24, pos.y + 24));
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
            if (ImGui::Button("##LandscapeP", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutLandscape)
                    switch_page_layout();
            }
            if (icon_landscape) {
                ImVec2 min = ImGui::GetItemRectMin();
                ImVec2 max = ImGui::GetItemRectMax();
                ImVec2 pos(min.x + (max.x - min.x - 24) * 0.5f, min.y + (max.y - min.y - 24) * 0.5f);
                drawList->AddImage((ImTextureID)icon_landscape, pos, ImVec2(pos.x + 24, pos.y + 24));
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
            const float landBarW = 110.0f;
            ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
            ImFont* font = ImGui::GetFont();
            float fontSize = ImGui::GetFontSize();
            float cos_a = cosf(IM_PI / 2.0f);
            float sin_a = sinf(IM_PI / 2.0f);
            ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);

            // --- Right Bar (background + buttons) ---
            float rightX = displayW - landBarW;
            ImGui::SetNextWindowPos(ImVec2(rightX, 0));
            ImGui::SetNextWindowSize(ImVec2(landBarW, displayH));
            ImGui::Begin("RightBar", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoNavFocus);

            if (ImGui::Button("##Back", ImVec2(82, 34))) {
                requestExit = true;
            }
            if (icon_back) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_back,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(28,28)),
                    ImVec2(28,28), cos_a, sin_a);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Back to library");
            }

            // Icon buttons at the bottom of the right bar
            float by = displayH - 190;
            ImGui::SetCursorPosY(by);
            if (ImGui::Button("##Text", ImVec2(82, 34))) { /* placeholder: text settings */ }
            if (icon_text) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_text,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(28,28)),
                    ImVec2(28,28), cos_a, sin_a);
            }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("##TOC", ImVec2(82, 34))) { /* placeholder: TOC */ }
            if (icon_toc) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_toc,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(28,28)),
                    ImVec2(28,28), cos_a, sin_a);
            }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("##Bookmark", ImVec2(82, 34))) { /* placeholder: bookmark */ }
            if (icon_bookmark) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_bookmark,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(28,28)),
                    ImVec2(28,28), cos_a, sin_a);
            }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("##Search", ImVec2(82, 34))) { /* placeholder: search */ }
            if (icon_search) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_search,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(28,28)),
                    ImVec2(28,28), cos_a, sin_a);
            }
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("##More", ImVec2(82, 34))) { /* placeholder: more */ }
            if (icon_more) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_more,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(28,28)),
                    ImVec2(28,28), cos_a, sin_a);
            }

            ImGui::End();
            drawList->AddLine(ImVec2(rightX, 0), ImVec2(rightX, displayH),
                ImGui::ColorConvertFloat4ToU32(sepCol), 1.0f);

            // --- Left Bar (background + buttons) ---
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
            if (ImGui::Button("##Portrait", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutPortrait)
                    switch_page_layout();
            }
            if (icon_portrait) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_portrait,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(24,24)),
                    ImVec2(24,24), cos_a, sin_a);
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
            if (ImGui::Button("##Landscape", ImVec2(btnW, 30))) {
                if (_currentPageLayout != BookPageLayoutLandscape)
                    switch_page_layout();
            }
            if (icon_landscape) {
                DrawRotatedIcon(fgDrawList, (ImTextureID)icon_landscape,
                    CenteredIconPos(ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax()), ImVec2(24,24)),
                    ImVec2(24,24), cos_a, sin_a);
            }
            if (_currentPageLayout == BookPageLayoutLandscape) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Landscape layout");
            }

            ImGui::End();
            drawList->AddLine(ImVec2(landBarW, 0), ImVec2(landBarW, displayH),
                ImGui::ColorConvertFloat4ToU32(sepCol), 1.0f);

            // --- Rotated text overlays (foreground draw-list) ---
            // Title on right bar
            std::string displayTitle = book_name;
            if (displayTitle.length() > 40) {
                displayTitle = displayTitle.substr(0, 37) + "...";
            }
            ImVec2 titleSize = ImGui::CalcTextSize(displayTitle.c_str());
            float titleStartY = 60.0f;
            float titleEndY = displayH - 200.0f;
            float titleAvailH = titleEndY - titleStartY;
            float titleH = titleSize.x;
            if (titleH > titleAvailH) {
                titleH = titleAvailH;
            }
            ImVec2 titleCenter(rightX + landBarW / 2.0f, titleStartY + titleH * 0.5f);
            ImVec2 titlePos(titleCenter.x - titleSize.x * 0.5f, titleCenter.y - titleSize.y * 0.5f);
            int vtx_start = fgDrawList->VtxBuffer.Size;
            fgDrawList->AddText(font, fontSize, titlePos, textCol, displayTitle.c_str());
            RotateDrawListVertices(fgDrawList, vtx_start, cos_a, sin_a, titleCenter);

            // Page info on left bar
            int cur = layout->current_page() + 1;
            int tot = layout->total_pages();
            int pct = tot > 0 ? (cur * 100 / tot) : 0;
            char infoBuf[64];
            snprintf(infoBuf, sizeof(infoBuf), "Pg %d/%d  %d%%", cur, tot, pct);
            std::string infoText = infoBuf;
            ImVec2 infoSize = ImGui::CalcTextSize(infoText.c_str());
            float infoStartY = 100.0f;
            ImVec2 infoCenter(landBarW / 2.0f, infoStartY + infoSize.x * 0.5f);
            ImVec2 infoPos(infoCenter.x - infoSize.x * 0.5f, infoCenter.y - infoSize.y * 0.5f);
            vtx_start = fgDrawList->VtxBuffer.Size;
            fgDrawList->AddText(font, fontSize, infoPos, textCol, infoText.c_str());
            RotateDrawListVertices(fgDrawList, vtx_start, cos_a, sin_a, infoCenter);
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
