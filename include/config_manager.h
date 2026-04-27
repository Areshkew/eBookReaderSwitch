#ifndef EBOOK_READER_CONFIG_MANAGER_H
#define EBOOK_READER_CONFIG_MANAGER_H

#include <string>
#include <unordered_map>

struct ConfigSettings {
    bool night_mode = false;
    std::string rotation = "portrait";
    std::string active_theme = "dark";
    std::string last_opened_book = "";
};

class ConfigManager {
public:
    bool Load(const char* path);
    void Save();

    const ConfigSettings& Settings() const { return settings_; }
    ConfigSettings& MutableSettings() { return settings_; }

    int GetSavedPage(const char* book_key) const;
    void SetSavedPage(const char* book_key, int page);

    void MarkDirty() { dirty_ = true; }
    bool IsDirty() const { return dirty_; }

private:
    std::string path_;
    ConfigSettings settings_;
    std::unordered_map<std::string, int> saved_pages_;
    bool dirty_ = false;
};

#endif
