#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace open_browser {

// Singleton ad and tracker blocker.
// Parses EasyList / EasyPrivacy compatible filter list syntax.
// Loaded synchronously from bundled lists on first use.
class AdBlocker {
public:
  static AdBlocker &instance();

  // Load all enabled filter lists from disk (bundled + user-added).
  void load_filter_lists();

  // Add a URL pointing to an additional filter list.
  void add_filter_list(const std::string &url);

  // Check whether a network request should be blocked.
  // request_url:  the URL being fetched
  // page_url:     the URL of the top-level page making the request
  bool should_block(const std::string &request_url,
                    const std::string &page_url) const;

  // Convenience: check tracker-specific lists only.
  bool should_block_tracker(const std::string &url) const;

  // Per-domain whitelist management.
  void add_whitelist_domain(const std::string &domain);
  void remove_whitelist_domain(const std::string &domain);
  bool is_whitelisted(const std::string &domain) const;

  std::size_t blocked_count() const { return blocked_count_; }
  void reset_count() { blocked_count_ = 0; }

private:
  AdBlocker() = default;
  AdBlocker(const AdBlocker &) = delete;
  AdBlocker &operator=(const AdBlocker &) = delete;

  // Parse a single filter list text block.
  void parse_filter_list(const std::string &content, bool tracker_only = false);

  // Extract domain from a full URL.
  std::string extract_domain(const std::string &url) const;

  // Bundled filter list location.
  std::string get_filter_list_dir() const;

  // Domain-based block lists (fast lookup)
  std::unordered_set<std::string> blocked_domains_;
  std::unordered_set<std::string> tracker_domains_;

  // Pattern-based rules (substring match; a subset of EasyList syntax)
  std::vector<std::string> block_patterns_;

  // User whitelist
  std::unordered_set<std::string> whitelist_;

  // Additional user-supplied filter list URLs
  std::vector<std::string> extra_list_urls_;

  mutable std::size_t blocked_count_ = 0;

  bool loaded_ = false;
};

} // namespace open_browser
