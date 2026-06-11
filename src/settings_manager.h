#pragma once

#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace open_browser {

using SettingValue = std::variant<bool, int, std::string, double>;

// Singleton settings store backed by ~/.config/open-browser/settings.json.
// All reads return safe defaults if the key is not present.
class SettingsManager {
public:
    static SettingsManager& instance();

    // Load from disk. Safe to call multiple times; subsequent calls reload.
    void load();

    // Persist to disk.
    void save() const;

    // Generic typed getter. Returns default-constructed T if key is missing
    // or the stored type doesn't match.
    template <typename T>
    T get(const std::string& key) const;

    // Generic typed setter. Fires registered callbacks after setting.
    template <typename T>
    void set(const std::string& key, const T& value);

    // Register a callback that fires when the given key changes.
    void on_change(const std::string& key,
                   std::function<void(const SettingValue&)> callback);

    // ── Convenience accessors (all have built-in defaults) ─────────────────
    std::string search_engine()       const;
    bool        hardware_acceleration() const;
    bool        dark_mode()           const;
    bool        block_ads()           const;
    bool        block_trackers()      const;
    bool        https_upgrade()       const;
    bool        save_passwords()      const;
    std::string download_path()       const;
    std::string homepage()            const;
    int         font_size()           const;
    bool        show_bookmarks_bar()  const;
    bool        smooth_scrolling()    const;
    bool        reduce_motion()       const;
    int         default_zoom()        const;
    std::string theme()               const; // "system" | "light" | "dark"

private:
    SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    std::string get_config_path() const;
    void notify_callbacks(const std::string& key) const;
    void apply_defaults();

    std::map<std::string, SettingValue> settings_;
    std::map<std::string, std::vector<std::function<void(const SettingValue&)>>> callbacks_;
};

} // namespace open_browser
