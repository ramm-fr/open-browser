// Open Browser — bookmark_manager.cpp

#include "bookmark_manager.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <glib.h>
#include <nlohmann/json.hpp>

namespace open_browser {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────

BookmarkManager& BookmarkManager::instance() {
    static BookmarkManager inst;
    return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────────────────────────────────────

void BookmarkManager::load() {
    const std::string path = get_data_path();
    if (!std::filesystem::exists(path)) return;

    try {
        std::ifstream f(path);
        json j = json::parse(f);

        // Bookmarks
        if (j.contains("bookmarks") && j["bookmarks"].is_array()) {
            for (const auto& item : j["bookmarks"]) {
                Bookmark bm;
                bm.id          = item.value("id",          int64_t{0});
                bm.url         = item.value("url",         std::string{});
                bm.title       = item.value("title",       std::string{});
                bm.folder      = item.value("folder",      std::string{});
                bm.created_at  = item.value("created_at",  std::time_t{0});
                bm.favicon_url = item.value("favicon_url", std::string{});
                if (bm.id >= next_bookmark_id_) next_bookmark_id_ = bm.id + 1;
                bookmarks_.push_back(std::move(bm));
            }
        }

        // Folders
        if (j.contains("folders") && j["folders"].is_array()) {
            for (const auto& item : j["folders"]) {
                BookmarkFolder folder;
                folder.id        = item.value("id",        int64_t{0});
                folder.name      = item.value("name",      std::string{});
                folder.parent_id = item.value("parent_id", int64_t{0});
                if (folder.id >= next_folder_id_) next_folder_id_ = folder.id + 1;
                folders_.push_back(std::move(folder));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[BookmarkManager] Load error: " << e.what() << "\n";
    }
}

void BookmarkManager::save() const {
    const std::string path = get_data_path();
    try {
        std::filesystem::create_directories(
            std::filesystem::path(path).parent_path()
        );

        json j;
        j["bookmarks"] = json::array();
        for (const auto& bm : bookmarks_) {
            j["bookmarks"].push_back({
                {"id",          bm.id},
                {"url",         bm.url},
                {"title",       bm.title},
                {"folder",      bm.folder},
                {"created_at",  bm.created_at},
                {"favicon_url", bm.favicon_url}
            });
        }

        j["folders"] = json::array();
        for (const auto& folder : folders_) {
            j["folders"].push_back({
                {"id",        folder.id},
                {"name",      folder.name},
                {"parent_id", folder.parent_id}
            });
        }

        std::ofstream f(path);
        f << j.dump(2) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[BookmarkManager] Save error: " << e.what() << "\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CRUD
// ─────────────────────────────────────────────────────────────────────────────

Bookmark BookmarkManager::add(const std::string& url,
                               const std::string& title,
                               const std::string& folder) {
    Bookmark bm;
    bm.id         = next_bookmark_id_++;
    bm.url        = url;
    bm.title      = title.empty() ? url : title;
    bm.folder     = folder;
    bm.created_at = std::time(nullptr);
    bookmarks_.push_back(bm);
    return bm;
}

bool BookmarkManager::remove(int64_t id) {
    auto it = std::find_if(bookmarks_.begin(), bookmarks_.end(),
        [id](const Bookmark& b) { return b.id == id; });
    if (it == bookmarks_.end()) return false;
    bookmarks_.erase(it);
    return true;
}

bool BookmarkManager::update(int64_t id,
                              const std::string& title,
                              const std::string& folder) {
    auto it = std::find_if(bookmarks_.begin(), bookmarks_.end(),
        [id](const Bookmark& b) { return b.id == id; });
    if (it == bookmarks_.end()) return false;
    it->title  = title;
    it->folder = folder;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Bookmark> BookmarkManager::get_all() const {
    return bookmarks_;
}

std::vector<Bookmark> BookmarkManager::get_by_folder(const std::string& folder) const {
    std::vector<Bookmark> result;
    for (const auto& bm : bookmarks_) {
        if (bm.folder == folder) result.push_back(bm);
    }
    return result;
}

std::vector<Bookmark> BookmarkManager::search(const std::string& query) const {
    const std::string lq = [&]() {
        std::string s = query;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }();

    std::vector<Bookmark> result;
    for (const auto& bm : bookmarks_) {
        auto lower = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        };
        if (lower(bm.title).find(lq) != std::string::npos ||
            lower(bm.url).find(lq)   != std::string::npos) {
            result.push_back(bm);
        }
    }
    return result;
}

std::optional<Bookmark> BookmarkManager::find_by_url(const std::string& url) const {
    auto it = std::find_if(bookmarks_.begin(), bookmarks_.end(),
        [&url](const Bookmark& b) { return b.url == url; });
    if (it == bookmarks_.end()) return std::nullopt;
    return *it;
}

bool BookmarkManager::is_bookmarked(const std::string& url) const {
    return find_by_url(url).has_value();
}

// ─────────────────────────────────────────────────────────────────────────────
// Folder management
// ─────────────────────────────────────────────────────────────────────────────

BookmarkFolder BookmarkManager::add_folder(const std::string& name,
                                            int64_t parent_id) {
    BookmarkFolder folder;
    folder.id        = next_folder_id_++;
    folder.name      = name;
    folder.parent_id = parent_id;
    folders_.push_back(folder);
    return folder;
}

bool BookmarkManager::remove_folder(int64_t id) {
    auto it = std::find_if(folders_.begin(), folders_.end(),
        [id](const BookmarkFolder& f) { return f.id == id; });
    if (it == folders_.end()) return false;
    folders_.erase(it);
    return true;
}

std::vector<BookmarkFolder> BookmarkManager::get_folders() const {
    return folders_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Path
// ─────────────────────────────────────────────────────────────────────────────

std::string BookmarkManager::get_data_path() const {
    const char* data_dir = g_get_user_data_dir();
    const std::string base = data_dir ? data_dir :
        (std::string(g_get_home_dir()) + "/.local/share");
    return base + "/open-browser/bookmarks.json";
}

} // namespace open_browser
