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
    bool load(const char* path);
    void save();

    const ConfigSettings& settings() const { return settings_; }
    ConfigSettings& mutable_settings() { return settings_; }

    int get_saved_page(const char* book_key) const;
    void set_saved_page(const char* book_key, int page);

    void mark_dirty() { dirty_ = true; }
    bool is_dirty() const { return dirty_; }

private:
    std::string path_;
    ConfigSettings settings_;
    std::unordered_map<std::string, int> saved_pages_;
    bool dirty_ = false;
};

#endif
