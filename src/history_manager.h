#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace open_browser {

struct HistoryEntry {
    int64_t     id          = 0;
    std::string url;
    std::string title;
    std::time_t visited_at  = 0;
    int         visit_count = 1;
};

// Singleton browsing history store backed by
// ~/.local/share/open-browser/history.json.
class HistoryManager {
public:
    static HistoryManager& instance();

    void load();
    void save() const;

    // Record a visit. If the URL already exists, its count is incremented and
    // the timestamp updated.
    void add_visit(const std::string& url, const std::string& title);
    void remove(int64_t id);
    void clear_all();

    // Clear entries whose visited_at falls within [from, to] (inclusive).
    void clear_range(std::time_t from, std::time_t to);

    // Returns up to `limit` entries ordered by most-recently visited.
    std::vector<HistoryEntry> get_recent(int limit = 100) const;

    // Case-insensitive substring search on URL and title.
    std::vector<HistoryEntry> search(const std::string& query) const;

    // Returns all entries visited on the day containing `date` (local time).
    std::vector<HistoryEntry> get_by_date(std::time_t date) const;

private:
    HistoryManager() = default;
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    std::string get_data_path() const;

    std::vector<HistoryEntry> entries_;
    int64_t next_id_ = 1;
};

} // namespace open_browser
