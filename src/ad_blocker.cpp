// Open Browser — ad_blocker.cpp
// EasyList-compatible filter list parser and request blocker.

#include "ad_blocker.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include <glib.h>

namespace open_browser {

// ─────────────────────────────────────────────────────────────────────────────
// Well-known tracker domains (embedded minimal list so the browser works
// out-of-the-box without downloading a filter list).
// ─────────────────────────────────────────────────────────────────────────────

static constexpr const char* kBuiltinTrackers[] = {
    "google-analytics.com",
    "googletagmanager.com",
    "googletagservices.com",
    "doubleclick.net",
    "googlesyndication.com",
    "facebook.com/tr",
    "connect.facebook.net",
    "analytics.twitter.com",
    "ads.twitter.com",
    "scorecardresearch.com",
    "quantserve.com",
    "pixel.quantserve.com",
    "amazon-adsystem.com",
    "adservice.google.com",
    "pagead2.googlesyndication.com",
    "stats.g.doubleclick.net",
    "mc.yandex.ru",
    "mc.yandex.com",
    "hotjar.com",
    "fullstory.com",
    "mixpanel.com",
    "segment.io",
    "segment.com",
    "amplitude.com",
    "intercom.io",
    "crisp.chat",
    "mouseflow.com",
    "clarity.ms",
    "cdn.jsdelivr.net/npm/jquery-3.6.0.min.js", // specific ad-tech
    "pagead.l.doubleclick.net",
    "adnxs.com",
    "rubiconproject.com",
    "pubmatic.com",
    "openx.net",
    "criteo.com",
    "criteo.net",
    "taboola.com",
    "outbrain.com",
    "moatads.com",
    "adsafeprotected.com",
    "casalemedia.com",
    "media.net",
    "bidswitch.net",
    "tremorhub.com",
    "appnexus.com",
    "contextweb.com",
    "advertising.com",
};

// ─────────────────────────────────────────────────────────────────────────────

AdBlocker& AdBlocker::instance() {
    static AdBlocker inst;
    return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
// Loading
// ─────────────────────────────────────────────────────────────────────────────

void AdBlocker::load_filter_lists() {
    if (loaded_) return;
    loaded_ = true;

    // 1. Load built-in tracker list
    for (const char* domain : kBuiltinTrackers) {
        tracker_domains_.insert(domain);
        blocked_domains_.insert(domain);
    }

    // 2. Try to load filter lists from disk (installed or dev path)
    const std::string filter_dir = get_filter_list_dir();
    if (!filter_dir.empty() && std::filesystem::exists(filter_dir)) {
        for (const auto& entry :
             std::filesystem::directory_iterator(filter_dir)) {
            if (entry.path().extension() == ".txt") {
                std::ifstream f(entry.path());
                std::string content((std::istreambuf_iterator<char>(f)),
                                     std::istreambuf_iterator<char>());
                const bool tracker_only =
                    entry.path().filename().string().find("privacy") !=
                    std::string::npos;
                parse_filter_list(content, tracker_only);
            }
        }
    }
}

void AdBlocker::add_filter_list(const std::string& url) {
    extra_list_urls_.push_back(url);
}

// ─────────────────────────────────────────────────────────────────────────────
// Parsing (subset of EasyList syntax)
// ─────────────────────────────────────────────────────────────────────────────

void AdBlocker::parse_filter_list(const std::string& content,
                                   bool tracker_only) {
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        // Skip blank lines and comments
        if (line.empty() || line[0] == '!' || line[0] == '[') continue;

        // Exception rules (@@) — skip for simplicity
        if (line.size() >= 2 && line[0] == '@' && line[1] == '@') continue;

        // Domain anchors: ||domain^
        if (line.size() >= 3 && line[0] == '|' && line[1] == '|') {
            std::string domain = line.substr(2);
            // Strip trailing ^ or / anchors and options
            for (char delim : {'^', '/', '$'}) {
                const size_t pos = domain.find(delim);
                if (pos != std::string::npos) domain = domain.substr(0, pos);
            }
            if (!domain.empty()) {
                blocked_domains_.insert(domain);
                if (tracker_only) tracker_domains_.insert(domain);
            }
            continue;
        }

        // Generic pattern: add to pattern list (substring match)
        // Only add patterns that don't contain regex-like characters
        // to keep matching fast.
        if (line.find('/') != std::string::npos &&
            line.find('*') == std::string::npos &&
            line.find('?') == std::string::npos &&
            line.find('[') == std::string::npos) {
            block_patterns_.push_back(line);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Blocking decision
// ─────────────────────────────────────────────────────────────────────────────

bool AdBlocker::should_block(const std::string& request_url,
                              const std::string& /*page_url*/) const {
    if (request_url.empty()) return false;

    const std::string req_domain = extract_domain(request_url);

    // Whitelist check
    if (is_whitelisted(req_domain)) return false;

    // Domain-based block
    if (blocked_domains_.count(req_domain)) {
        ++blocked_count_;
        return true;
    }

    // Check subdomains (e.g. sub.ads.com should be caught by ads.com)
    std::string sub = req_domain;
    while (true) {
        const size_t dot = sub.find('.');
        if (dot == std::string::npos) break;
        sub = sub.substr(dot + 1);
        if (blocked_domains_.count(sub)) {
            ++blocked_count_;
            return true;
        }
    }

    // Pattern-based block (substring search in full URL)
    for (const auto& pattern : block_patterns_) {
        if (request_url.find(pattern) != std::string::npos) {
            ++blocked_count_;
            return true;
        }
    }

    return false;
}

bool AdBlocker::should_block_tracker(const std::string& url) const {
    if (url.empty()) return false;
    const std::string domain = extract_domain(url);

    if (is_whitelisted(domain)) return false;
    if (tracker_domains_.count(domain)) {
        ++blocked_count_;
        return true;
    }
    std::string sub = domain;
    while (true) {
        const size_t dot = sub.find('.');
        if (dot == std::string::npos) break;
        sub = sub.substr(dot + 1);
        if (tracker_domains_.count(sub)) {
            ++blocked_count_;
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Whitelist
// ─────────────────────────────────────────────────────────────────────────────

void AdBlocker::add_whitelist_domain(const std::string& domain) {
    whitelist_.insert(domain);
}

void AdBlocker::remove_whitelist_domain(const std::string& domain) {
    whitelist_.erase(domain);
}

bool AdBlocker::is_whitelisted(const std::string& domain) const {
    if (whitelist_.count(domain)) return true;
    // Check parent domains
    std::string sub = domain;
    while (true) {
        const size_t dot = sub.find('.');
        if (dot == std::string::npos) break;
        sub = sub.substr(dot + 1);
        if (whitelist_.count(sub)) return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Utilities
// ─────────────────────────────────────────────────────────────────────────────

std::string AdBlocker::extract_domain(const std::string& url) const {
    std::string s = url;

    // Strip scheme
    for (const char* scheme : {"https://", "http://", "ftp://"}) {
        const std::string_view sv(scheme);
        if (s.size() > sv.size() && s.compare(0, sv.size(), sv) == 0) {
            s = s.substr(sv.size());
            break;
        }
    }

    // Strip path, query, fragment
    for (char delim : {'/', '?', '#'}) {
        const size_t pos = s.find(delim);
        if (pos != std::string::npos) s = s.substr(0, pos);
    }

    // Strip port
    const size_t colon = s.rfind(':');
    if (colon != std::string::npos) {
        const std::string port = s.substr(colon + 1);
        if (!port.empty() && std::all_of(port.begin(), port.end(), ::isdigit)) {
            s = s.substr(0, colon);
        }
    }

    // Lowercase
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string AdBlocker::get_filter_list_dir() const {
    // Installed location
    const char* data_dirs = g_get_system_data_dirs()
        ? g_get_system_data_dirs()[0] : nullptr;
    if (data_dirs) {
        std::filesystem::path p =
            std::filesystem::path(data_dirs) / "open-browser" / "filter-lists";
        if (std::filesystem::exists(p)) return p.string();
    }

    // Development: next to the binary
    const std::filesystem::path exe_dir =
        std::filesystem::canonical("/proc/self/exe").parent_path();
    std::filesystem::path dev_path = exe_dir / "filter-lists";
    if (std::filesystem::exists(dev_path)) return dev_path.string();

    return "";
}

} // namespace open_browser
