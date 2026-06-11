// Open Browser — settings_manager.cpp
// JSON-backed singleton settings store.

#include "settings_manager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <glib.h>
#include <nlohmann/json.hpp>

namespace open_browser {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────

SettingsManager& SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// Defaults
// ─────────────────────────────────────────────────────────────────────────────

void SettingsManager::apply_defaults() {
    auto set_default = [this](const std::string& key, const SettingValue& val) {
        if (settings_.find(key) == settings_.end()) {
            settings_[key] = val;
        }
    };

    // General
    set_default("homepage",              std::string("openbrowser://newtab"));
    set_default("search_engine",         std::string("https://search.brave.com/search?q="));
    set_default("theme",                 std::string("system"));
    set_default("font_size",             16);
    set_default("default_zoom",          100);
    set_default("show_bookmarks_bar",    false);
    set_default("smooth_scrolling",      true);
    set_default("reduce_motion",         false);

    // Performance
    set_default("hardware_acceleration", true);
    set_default("sleeping_tabs",         true);
    set_default("memory_saver",          false);

    // Privacy
    set_default("block_ads",             true);
    set_default("block_trackers",        true);
    set_default("https_upgrade",         true);
    set_default("clear_on_exit",         false);
    set_default("dark_mode",             false);

    // Passwords
    set_default("save_passwords",        true);
    set_default("autofill_passwords",    true);

    // Downloads
    const char* home_dir = g_get_home_dir();
    const std::string default_dl = home_dir
        ? std::string(home_dir) + "/Downloads"
        : "/tmp";
    set_default("download_path",         default_dl);
    set_default("ask_download_location", false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Load / Save
// ─────────────────────────────────────────────────────────────────────────────

void SettingsManager::load() {
    apply_defaults();

    const std::string path = get_config_path();
    if (!std::filesystem::exists(path)) {
        // No file yet — defaults are already applied.
        return;
    }

    try {
        std::ifstream f(path);
        json j = json::parse(f);

        for (auto& [key, val] : j.items()) {
            if (val.is_boolean()) {
                settings_[key] = val.get<bool>();
            } else if (val.is_number_integer()) {
                settings_[key] = val.get<int>();
            } else if (val.is_number_float()) {
                settings_[key] = val.get<double>();
            } else if (val.is_string()) {
                settings_[key] = val.get<std::string>();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[SettingsManager] Failed to load settings: "
                  << e.what() << "\n";
    }
}

void SettingsManager::save() const {
    const std::string path = get_config_path();
    try {
        std::filesystem::create_directories(
            std::filesystem::path(path).parent_path()
        );

        json j = json::object();
        for (const auto& [key, val] : settings_) {
            std::visit([&](const auto& v) { j[key] = v; }, val);
        }

        std::ofstream f(path);
        f << j.dump(2) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[SettingsManager] Failed to save settings: "
                  << e.what() << "\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Generic get / set
// ─────────────────────────────────────────────────────────────────────────────

template <typename T>
T SettingsManager::get(const std::string& key) const {
    auto it = settings_.find(key);
    if (it == settings_.end()) return T{};
    const T* ptr = std::get_if<T>(&it->second);
    return ptr ? *ptr : T{};
}

template <typename T>
void SettingsManager::set(const std::string& key, const T& value) {
    settings_[key] = value;
    notify_callbacks(key);
}

// Explicit instantiations for the four supported types
template bool        SettingsManager::get<bool>(const std::string&) const;
template int         SettingsManager::get<int>(const std::string&) const;
template std::string SettingsManager::get<std::string>(const std::string&) const;
template double      SettingsManager::get<double>(const std::string&) const;

template void SettingsManager::set<bool>(const std::string&, const bool&);
template void SettingsManager::set<int>(const std::string&, const int&);
template void SettingsManager::set<std::string>(const std::string&, const std::string&);
template void SettingsManager::set<double>(const std::string&, const double&);

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────

void SettingsManager::on_change(const std::string& key,
                                 std::function<void(const SettingValue&)> callback) {
    callbacks_[key].push_back(std::move(callback));
}

void SettingsManager::notify_callbacks(const std::string& key) const {
    auto it = callbacks_.find(key);
    if (it == callbacks_.end()) return;
    const SettingValue& val = settings_.at(key);
    for (const auto& cb : it->second) {
        cb(val);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Convenience accessors
// ─────────────────────────────────────────────────────────────────────────────

std::string SettingsManager::search_engine() const {
    return get<std::string>("search_engine");
}
bool SettingsManager::hardware_acceleration() const {
    return get<bool>("hardware_acceleration");
}
bool SettingsManager::dark_mode() const {
    return get<bool>("dark_mode");
}
bool SettingsManager::block_ads() const {
    return get<bool>("block_ads");
}
bool SettingsManager::block_trackers() const {
    return get<bool>("block_trackers");
}
bool SettingsManager::https_upgrade() const {
    return get<bool>("https_upgrade");
}
bool SettingsManager::save_passwords() const {
    return get<bool>("save_passwords");
}
std::string SettingsManager::download_path() const {
    return get<std::string>("download_path");
}
std::string SettingsManager::homepage() const {
    return get<std::string>("homepage");
}
int SettingsManager::font_size() const {
    return get<int>("font_size");
}
bool SettingsManager::show_bookmarks_bar() const {
    return get<bool>("show_bookmarks_bar");
}
bool SettingsManager::smooth_scrolling() const {
    return get<bool>("smooth_scrolling");
}
bool SettingsManager::reduce_motion() const {
    return get<bool>("reduce_motion");
}
int SettingsManager::default_zoom() const {
    return get<int>("default_zoom");
}
std::string SettingsManager::theme() const {
    return get<std::string>("theme");
}

// ─────────────────────────────────────────────────────────────────────────────
// Path helper
// ─────────────────────────────────────────────────────────────────────────────

std::string SettingsManager::get_config_path() const {
    const char* config_dir = g_get_user_config_dir();
    const std::string base = config_dir ? config_dir : (
        std::string(g_get_home_dir()) + "/.config"
    );
    return base + "/open-browser/settings.json";
}

} // namespace open_browser
