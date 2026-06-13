// Open Browser — history_manager.cpp

#include "history_manager.h"

#include <glib.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace open_browser {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────

HistoryManager &HistoryManager::instance() {
  static HistoryManager inst;
  return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence
// ─────────────────────────────────────────────────────────────────────────────

void HistoryManager::load() {
  const std::string path = get_data_path();
  if (!std::filesystem::exists(path))
    return;

  try {
    std::ifstream f(path);
    json j = json::parse(f);

    if (j.contains("entries") && j["entries"].is_array()) {
      for (const auto &item : j["entries"]) {
        HistoryEntry entry;
        entry.id = item.value("id", int64_t{0});
        entry.url = item.value("url", std::string{});
        entry.title = item.value("title", std::string{});
        entry.visited_at = item.value("visited_at", std::time_t{0});
        entry.visit_count = item.value("visit_count", 1);
        if (entry.id >= next_id_)
          next_id_ = entry.id + 1;
        entries_.push_back(std::move(entry));
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[HistoryManager] Load error: " << e.what() << "\n";
  }
}

void HistoryManager::save() const {
  const std::string path = get_data_path();
  try {
    std::filesystem::create_directories(
        std::filesystem::path(path).parent_path());

    json j;
    j["entries"] = json::array();
    for (const auto &entry : entries_) {
      j["entries"].push_back({{"id", entry.id},
                              {"url", entry.url},
                              {"title", entry.title},
                              {"visited_at", entry.visited_at},
                              {"visit_count", entry.visit_count}});
    }

    std::ofstream f(path);
    f << j.dump(2) << "\n";
  } catch (const std::exception &e) {
    std::cerr << "[HistoryManager] Save error: " << e.what() << "\n";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Mutation
// ─────────────────────────────────────────────────────────────────────────────

void HistoryManager::add_visit(const std::string &url,
                               const std::string &title) {
  if (url.empty())
    return;

  // Check if URL already exists
  auto it =
      std::find_if(entries_.begin(), entries_.end(),
                   [&url](const HistoryEntry &e) { return e.url == url; });

  if (it != entries_.end()) {
    it->visited_at = std::time(nullptr);
    it->visit_count++;
    if (!title.empty())
      it->title = title;
  } else {
    HistoryEntry entry;
    entry.id = next_id_++;
    entry.url = url;
    entry.title = title.empty() ? url : title;
    entry.visited_at = std::time(nullptr);
    entry.visit_count = 1;
    entries_.push_back(std::move(entry));
  }
}

void HistoryManager::remove(int64_t id) {
  auto it = std::find_if(entries_.begin(), entries_.end(),
                         [id](const HistoryEntry &e) { return e.id == id; });
  if (it != entries_.end())
    entries_.erase(it);
}

void HistoryManager::clear_all() {
  entries_.clear();
  next_id_ = 1;
}

void HistoryManager::clear_range(std::time_t from, std::time_t to) {
  entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                [from, to](const HistoryEntry &e) {
                                  return e.visited_at >= from &&
                                         e.visited_at <= to;
                                }),
                 entries_.end());
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

std::vector<HistoryEntry> HistoryManager::get_recent(int limit) const {
  std::vector<HistoryEntry> sorted = entries_;
  std::sort(sorted.begin(), sorted.end(),
            [](const HistoryEntry &a, const HistoryEntry &b) {
              return a.visited_at > b.visited_at;
            });
  if (static_cast<int>(sorted.size()) > limit) {
    sorted.resize(static_cast<size_t>(limit));
  }
  return sorted;
}

std::vector<HistoryEntry>
HistoryManager::search(const std::string &query) const {
  auto to_lower = [](std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
  };
  const std::string lq = to_lower(query);

  std::vector<HistoryEntry> result;
  for (const auto &entry : entries_) {
    if (to_lower(entry.title).find(lq) != std::string::npos ||
        to_lower(entry.url).find(lq) != std::string::npos) {
      result.push_back(entry);
    }
  }
  // Sort by most recent
  std::sort(result.begin(), result.end(),
            [](const HistoryEntry &a, const HistoryEntry &b) {
              return a.visited_at > b.visited_at;
            });
  return result;
}

std::vector<HistoryEntry> HistoryManager::get_by_date(std::time_t date) const {
  // Compute start and end of the day (local time)
  struct tm day_tm = *std::localtime(&date);
  day_tm.tm_hour = 0;
  day_tm.tm_min = 0;
  day_tm.tm_sec = 0;
  const std::time_t day_start = std::mktime(&day_tm);
  const std::time_t day_end = day_start + 86400;

  std::vector<HistoryEntry> result;
  for (const auto &entry : entries_) {
    if (entry.visited_at >= day_start && entry.visited_at < day_end) {
      result.push_back(entry);
    }
  }
  std::sort(result.begin(), result.end(),
            [](const HistoryEntry &a, const HistoryEntry &b) {
              return a.visited_at > b.visited_at;
            });
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Path
// ─────────────────────────────────────────────────────────────────────────────

std::string HistoryManager::get_data_path() const {
  const char *data_dir = g_get_user_data_dir();
  const std::string base =
      data_dir ? data_dir : (std::string(g_get_home_dir()) + "/.local/share");
  return base + "/open-browser/history.json";
}

} // namespace open_browser
