#include "config_manager.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <cstdio>
#include <cstring>

extern "C" {
#include "logger.h"
}

bool ConfigManager::load(const char* path) {
    path_ = path;

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        LOG_I("Config file not found, using defaults: %s", path);
        dirty_ = true;
        return true;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (len <= 0) {
        fclose(fp);
        dirty_ = true;
        return true;
    }

    char* buf = new char[len + 1];
    fread(buf, 1, len, fp);
    buf[len] = '\0';
    fclose(fp);

    rapidjson::Document doc;
    doc.ParseInsitu(buf);

    if (doc.HasParseError()) {
        LOG_W("Config parse error, using defaults");
        delete[] buf;
        dirty_ = true;
        return true;
    }

    if (doc.HasMember("settings") && doc["settings"].IsObject()) {
        const auto& s = doc["settings"];
        if (s.HasMember("night_mode") && s["night_mode"].IsBool())
            settings_.night_mode = s["night_mode"].GetBool();
        if (s.HasMember("rotation") && s["rotation"].IsString())
            settings_.rotation = s["rotation"].GetString();
        if (s.HasMember("active_theme") && s["active_theme"].IsString())
            settings_.active_theme = s["active_theme"].GetString();
        if (s.HasMember("last_opened_book") && s["last_opened_book"].IsString())
            settings_.last_opened_book = s["last_opened_book"].GetString();
    }

    if (doc.HasMember("saved_pages") && doc["saved_pages"].IsObject()) {
        const auto& sp = doc["saved_pages"];
        for (auto it = sp.MemberBegin(); it != sp.MemberEnd(); ++it) {
            if (it->value.IsInt()) {
                saved_pages_[it->name.GetString()] = it->value.GetInt();
            }
        }
    }

    delete[] buf;
    LOG_I("Config loaded: %s", path);
    return true;
}

void ConfigManager::save() {
    if (path_.empty()) return;

    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    rapidjson::Value settings(rapidjson::kObjectType);
    settings.AddMember("night_mode", settings_.night_mode, alloc);
    settings.AddMember("rotation",
        rapidjson::Value(settings_.rotation.c_str(), alloc), alloc);
    settings.AddMember("active_theme",
        rapidjson::Value(settings_.active_theme.c_str(), alloc), alloc);
    settings.AddMember("last_opened_book",
        rapidjson::Value(settings_.last_opened_book.c_str(), alloc), alloc);
    doc.AddMember("settings", settings, alloc);

    rapidjson::Value pages(rapidjson::kObjectType);
    for (const auto& kv : saved_pages_) {
        pages.AddMember(
            rapidjson::Value(kv.first.c_str(), alloc),
            rapidjson::Value(kv.second),
            alloc);
    }
    doc.AddMember("saved_pages", pages, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    doc.Accept(writer);

    FILE* fp = fopen(path_.c_str(), "w");
    if (!fp) {
        LOG_W("Failed to write config: %s", path_.c_str());
        return;
    }
    fwrite(sb.GetString(), 1, sb.GetSize(), fp);
    fclose(fp);
    dirty_ = false;
}

int ConfigManager::get_saved_page(const char* book_key) const {
    auto it = saved_pages_.find(book_key);
    return (it != saved_pages_.end()) ? it->second : 0;
}

void ConfigManager::set_saved_page(const char* book_key, int page) {
    saved_pages_[book_key] = page;
    dirty_ = true;
    save();
}
