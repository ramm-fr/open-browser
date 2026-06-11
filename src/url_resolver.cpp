// Open Browser — url_resolver.cpp

#include "url_resolver.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace open_browser {

namespace {

// Well-known URL schemes that should be navigated to directly.
constexpr std::array<std::string_view, 10> kKnownSchemes = {
    "http://", "https://", "ftp://", "ftps://",
    "file://", "data:", "blob:",
    "openbrowser://", "about:", "javascript:"
};

// Returns true if the string starts with one of the known schemes.
bool has_known_scheme(const std::string& s) {
    for (const auto& scheme : kKnownSchemes) {
        if (s.size() >= scheme.size() &&
            s.compare(0, scheme.size(), scheme) == 0) {
            return true;
        }
    }
    return false;
}

// Strips leading/trailing whitespace.
std::string trim(const std::string& s) {
    const std::string ws = " \t\n\r\f\v";
    const size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    const size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

// Returns true if the string looks like a hostname segment (letters, digits,
// hyphens, but not starting/ending with a hyphen).
bool is_valid_hostname_label(const std::string& label) {
    if (label.empty()) return false;
    if (label.front() == '-' || label.back() == '-') return false;
    return std::all_of(label.begin(), label.end(),
        [](unsigned char c) { return std::isalnum(c) || c == '-'; });
}

// Returns true if the string looks like a domain name (e.g. "google.com").
bool looks_like_domain(const std::string& s) {
    // Must contain at least one dot
    const size_t dot = s.find('.');
    if (dot == std::string::npos || dot == 0 || dot == s.size() - 1) {
        return false;
    }
    // Must not contain spaces
    if (s.find(' ') != std::string::npos) return false;

    // Split on dots and validate each label
    std::string part;
    std::istringstream stream(s);
    while (std::getline(stream, part, '.')) {
        if (!is_valid_hostname_label(part)) return false;
    }
    return true;
}

// Returns true if the string is a localhost reference (with optional port).
bool is_localhost(const std::string& s) {
    if (s == "localhost") return true;
    if (s.starts_with("localhost:")) return true;
    // IPv4
    if (s.starts_with("127.") || s.starts_with("0.0.0.0")) return true;
    // IPv6
    if (s.starts_with("[::1]") || s.starts_with("::1")) return true;
    return false;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

std::string UrlResolver::resolve(const std::string& input,
                                  const std::string& search_engine) {
    const std::string trimmed = trim(input);
    if (trimmed.empty()) return search_engine;

    // 1. Already has a known scheme — use as-is.
    if (has_known_scheme(trimmed)) {
        return trimmed;
    }

    // 2. Looks like a URL without a scheme.
    if (looks_like_url(trimmed)) {
        return "https://" + trimmed;
    }

    // 3. Treat as a search query.
    return search_engine + encode_search_query(trimmed);
}

bool UrlResolver::is_internal_url(const std::string& url) {
    return url.starts_with("openbrowser://");
}

bool UrlResolver::looks_like_url(const std::string& input) {
    if (input.empty()) return false;

    // Already has a scheme
    if (has_known_scheme(input)) return true;

    // localhost / 127.0.0.1 style
    if (is_localhost(input)) return true;

    // Contains a slash after what looks like a hostname — likely a path
    const size_t slash = input.find('/');
    const std::string host_part = (slash != std::string::npos)
        ? input.substr(0, slash)
        : input;

    // Strip port
    const std::string domain_part = [&]() -> std::string {
        const size_t colon = host_part.rfind(':');
        if (colon != std::string::npos) {
            const std::string port_str = host_part.substr(colon + 1);
            if (std::all_of(port_str.begin(), port_str.end(), ::isdigit)) {
                return host_part.substr(0, colon);
            }
        }
        return host_part;
    }();

    return looks_like_domain(domain_part);
}

std::string UrlResolver::encode_search_query(const std::string& query) {
    std::ostringstream encoded;
    encoded << std::hex << std::uppercase;

    for (const unsigned char c : query) {
        if (std::isalnum(c) ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << static_cast<char>(c);
        } else if (c == ' ') {
            encoded << '+';
        } else {
            encoded << '%' << std::setw(2) << std::setfill('0')
                    << static_cast<int>(c);
        }
    }
    return encoded.str();
}

} // namespace open_browser
