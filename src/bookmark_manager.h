#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <vector>

namespace open_browser {

struct Bookmark {
    int64_t     id          = 0;
    std::string url;
    std::string title;
    std::string folder;
    std::time_t created_at  = 0;
    std::string favicon_url;
};

struct BookmarkFolder {
    int64_t     id        = 0;
    std::string name;
    int64_t     parent_id = 0;
};

// Singleton bookmark store backed by
// ~/.local/share/open-browser/bookmarks.json.
class BookmarkManager {
public:
    static BookmarkManager& instance();

    void load();
    void save() const;

    Bookmark add(const std::string& url,
                 const std::string& title,
                 const std::string& folder = "");
    bool remove(int64_t id);
    bool update(int64_t id,
                const std::string& title,
                const std::string& folder);

    std::vector<Bookmark> get_all()            const;
    std::vector<Bookmark> get_by_folder(const std::string& folder) const;
    std::vector<Bookmark> search(const std::string& query) const;
    std::optional<Bookmark> find_by_url(const std::string& url) const;
    bool is_bookmarked(const std::string& url) const;

    BookmarkFolder add_folder(const std::string& name, int64_t parent_id = 0);
    bool remove_folder(int64_t id);
    std::vector<BookmarkFolder> get_folders() const;

private:
    BookmarkManager() = default;
    BookmarkManager(const BookmarkManager&) = delete;
    BookmarkManager& operator=(const BookmarkManager&) = delete;

    std::string get_data_path() const;

    std::vector<Bookmark>       bookmarks_;
    std::vector<BookmarkFolder> folders_;
    int64_t next_bookmark_id_ = 1;
    int64_t next_folder_id_   = 1;
};

} // namespace open_browser
