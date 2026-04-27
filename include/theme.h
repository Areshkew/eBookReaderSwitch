#ifndef EBOOK_READER_THEME_H
#define EBOOK_READER_THEME_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>

struct ThemeColor {
    Uint8 r, g, b, a;
};

struct ThemeNightOverlay {
    Uint8 r, g, b;
    Uint8 alpha;
};

struct ThemeImGuiColor {
    float r, g, b, a;
};

struct Theme {
    std::string name;

    ThemeColor background;
    ThemeColor text;
    ThemeColor accent;
    ThemeColor page_bg;
    ThemeColor page_bg_light;
    ThemeColor status_bar;
    ThemeColor selector;
    ThemeColor hint;
    ThemeColor title;
    ThemeColor separator;
    ThemeColor icon_tint;
    ThemeColor icon_bg;
    ThemeColor badge_bg;
    ThemeColor badge_text;
    ThemeColor bar_bg;
    ThemeColor separator_bar;

    ThemeNightOverlay night_overlay;

    std::unordered_map<std::string, ThemeImGuiColor> imgui_colors;
};

class ThemeManager {
public:
    static void Init();
    static void Shutdown();
    static ThemeManager& Instance();

    bool LoadTheme(const char* theme_name);
    const Theme& CurrentTheme() const { return current_; }
    const std::string& CurrentThemeName() const { return current_name_; }
    bool IsDark() const;
    std::vector<std::string> ListThemes() const;

    static Theme DarkDefault();
    static Theme LightDefault();

private:
    ThemeManager() = default;
    Theme current_;
    std::string current_name_;
    bool initialized_ = false;
};

#endif
