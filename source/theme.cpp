#include "theme.h"
#include <rapidjson/document.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

extern "C" {
#include "logger.h"
#include "paths.h"
#include "fs.h"
}

static ThemeManager* s_instance = nullptr;

void ThemeManager::init() {
    if (!s_instance) {
        s_instance = new ThemeManager();
        s_instance->initialized_ = true;
    }
}

void ThemeManager::shutdown() {
    delete s_instance;
    s_instance = nullptr;
}

ThemeManager& ThemeManager::instance() {
    return *s_instance;
}

static ThemeColor parse_color(const char* hex) {
    ThemeColor c = {0, 0, 0, 255};
    if (!hex) return c;
    size_t len = strlen(hex);
    unsigned int val = 0;
    sscanf(hex, "%x", &val);
    if (len >= 6) {
        c.r = (val >> 16) & 0xFF;
        c.g = (val >> 8) & 0xFF;
        c.b = val & 0xFF;
    }
    if (len >= 8) {
        c.a = val & 0xFF;
    }
    return c;
}

static ThemeImGuiColor parse_imgui_color(const rapidjson::Value& arr) {
    ThemeImGuiColor c = {0, 0, 0, 1};
    if (arr.IsArray() && arr.Size() >= 3) {
        c.r = arr[0].GetFloat();
        c.g = arr[1].GetFloat();
        c.b = arr[2].GetFloat();
        if (arr.Size() >= 4) c.a = arr[3].GetFloat();
    }
    return c;
}

static Theme load_theme_from_json(const char* path) {
    Theme t = ThemeManager::dark_default();

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        LOG_W("Theme file not found: %s", path);
        return t;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len <= 0) { fclose(fp); return t; }

    char* buf = new char[len + 1];
    fread(buf, 1, len, fp);
    buf[len] = '\0';
    fclose(fp);

    rapidjson::Document doc;
    doc.ParseInsitu(buf);
    if (doc.HasParseError()) {
        LOG_W("Theme parse error: %s", path);
        delete[] buf;
        return t;
    }

    if (doc.HasMember("name") && doc["name"].IsString())
        t.name = doc["name"].GetString();

    if (doc.HasMember("colors") && doc["colors"].IsObject()) {
        const auto& c = doc["colors"];
        if (c.HasMember("background"))       t.background       = parse_color(c["background"].GetString());
        if (c.HasMember("text"))             t.text             = parse_color(c["text"].GetString());
        if (c.HasMember("accent"))           t.accent           = parse_color(c["accent"].GetString());
        if (c.HasMember("page_bg"))          t.page_bg          = parse_color(c["page_bg"].GetString());
        if (c.HasMember("page_bg_light"))    t.page_bg_light    = parse_color(c["page_bg_light"].GetString());
        if (c.HasMember("status_bar"))       t.status_bar       = parse_color(c["status_bar"].GetString());
        if (c.HasMember("selector"))         t.selector         = parse_color(c["selector"].GetString());
        if (c.HasMember("hint"))             t.hint             = parse_color(c["hint"].GetString());
        if (c.HasMember("title"))            t.title            = parse_color(c["title"].GetString());
        if (c.HasMember("separator"))        t.separator        = parse_color(c["separator"].GetString());
        if (c.HasMember("icon_tint"))        t.icon_tint        = parse_color(c["icon_tint"].GetString());
        if (c.HasMember("icon_bg"))          t.icon_bg          = parse_color(c["icon_bg"].GetString());
        if (c.HasMember("badge_bg"))         t.badge_bg         = parse_color(c["badge_bg"].GetString());
        if (c.HasMember("badge_text"))       t.badge_text       = parse_color(c["badge_text"].GetString());
        if (c.HasMember("bar_bg"))           t.bar_bg           = parse_color(c["bar_bg"].GetString());
        if (c.HasMember("separator_bar"))    t.separator_bar    = parse_color(c["separator_bar"].GetString());
    }

    if (doc.HasMember("night_overlay") && doc["night_overlay"].IsObject()) {
        const auto& n = doc["night_overlay"];
        if (n.HasMember("color")) {
            ThemeColor nc = parse_color(n["color"].GetString());
            t.night_overlay.r = nc.r;
            t.night_overlay.g = nc.g;
            t.night_overlay.b = nc.b;
        }
        if (n.HasMember("alpha") && n["alpha"].IsInt())
            t.night_overlay.alpha = (Uint8)n["alpha"].GetInt();
    }

    if (doc.HasMember("imgui") && doc["imgui"].IsObject()) {
        const auto& ig = doc["imgui"];
        for (auto it = ig.MemberBegin(); it != ig.MemberEnd(); ++it) {
            t.imgui_colors[it->name.GetString()] = parse_imgui_color(it->value);
        }
    }

    delete[] buf;
    LOG_I("Theme loaded: %s (%s)", t.name.c_str(), path);
    return t;
}

bool ThemeManager::load_theme(const char* theme_name) {
    char sd_path[512];
    snprintf(sd_path, sizeof(sd_path), "%s/%s/theme.json", THEMES_DIR, theme_name);

    char romfs_path[512];
    snprintf(romfs_path, sizeof(romfs_path), "%s/%s/theme.json", THEMES_ROMFS_BASE, theme_name);

    if (FS_FileExists(sd_path)) {
        current_ = load_theme_from_json(sd_path);
    } else if (FS_FileExists(romfs_path)) {
        current_ = load_theme_from_json(romfs_path);
    } else {
        LOG_W("Theme not found: %s, falling back to default", theme_name);
        current_ = (strcmp(theme_name, "light") == 0) ? light_default() : dark_default();
    }
    current_name_ = theme_name;
    return true;
}

bool ThemeManager::is_dark() const {
    float lum = 0.299f * current_.background.r + 0.587f * current_.background.g + 0.114f * current_.background.b;
    return lum < 128.0f;
}

std::vector<std::string> ThemeManager::list_themes() const {
    std::vector<std::string> result;
    if (!fs::exists(THEMES_DIR)) return result;
    for (const auto& entry : fs::directory_iterator(THEMES_DIR)) {
        if (entry.is_directory()) {
            std::string name = entry.path().filename().string();
            char check[1024];
            snprintf(check, sizeof(check), "%s/%s/theme.json", THEMES_DIR, name.c_str());
            if (FS_FileExists(check)) {
                result.push_back(name);
            }
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

Theme ThemeManager::dark_default() {
    Theme t;
    t.name = "Dark";
    t.background       = {0, 0, 0, 255};
    t.text             = {240, 240, 240, 255};
    t.accent           = {0, 209, 255, 255};
    t.page_bg          = {0, 0, 0, 255};
    t.page_bg_light    = {255, 255, 255, 255};
    t.status_bar       = {163, 20, 20, 255};
    t.selector         = {76, 76, 76, 255};
    t.hint             = {70, 70, 70, 255};
    t.title            = {0, 150, 136, 255};
    t.separator        = {76, 76, 76, 255};
    t.icon_tint        = {240, 240, 240, 255};
    t.icon_bg          = {200, 200, 200, 255};
    t.badge_bg         = {60, 60, 60, 200};
    t.badge_text       = {160, 160, 160, 255};
    t.bar_bg           = {36, 36, 36, 255};
    t.separator_bar    = {77, 77, 77, 255};
    t.night_overlay    = {255, 140, 0, 70};
    return t;
}

Theme ThemeManager::light_default() {
    Theme t;
    t.name = "Light";
    t.background       = {255, 255, 255, 255};
    t.text             = {25, 25, 25, 255};
    t.accent           = {0, 140, 191, 255};
    t.page_bg          = {255, 255, 255, 255};
    t.page_bg_light    = {255, 255, 255, 255};
    t.status_bar       = {240, 43, 43, 255};
    t.selector         = {220, 220, 220, 255};
    t.hint             = {210, 210, 210, 255};
    t.title            = {30, 136, 229, 255};
    t.separator        = {153, 153, 153, 255};
    t.icon_tint        = {255, 255, 255, 255};
    t.icon_bg          = {0, 0, 0, 0};
    t.badge_bg         = {200, 200, 200, 200};
    t.badge_text       = {80, 80, 80, 255};
    t.bar_bg           = {219, 219, 219, 255};
    t.separator_bar    = {153, 153, 153, 255};
    t.night_overlay    = {255, 140, 0, 70};
    return t;
}
