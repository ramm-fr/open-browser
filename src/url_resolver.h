#pragma once
#include <string>

namespace open_browser {

// Resolves user input (address bar text) into a navigatable URL.
//
// Resolution rules:
//   1. openbrowser:// internal pages are returned as-is.
//   2. Strings that look like URLs (contain a dot, start with a known scheme,
//      or are "localhost") get https:// prepended if they have no scheme.
//   3. Everything else is treated as a search query and encoded into the
//      configured search engine URL.
class UrlResolver {
public:
  // Resolve user input to a final URL.
  // search_engine should be a URL template ending with "?q=" (or equivalent).
  static std::string resolve(
      const std::string &input,
      const std::string &search_engine = "https://search.brave.com/search?q=");

  // Returns true if the URL uses the openbrowser:// scheme.
  static bool is_internal_url(const std::string &url);

  // Returns true if the string looks like a URL rather than a search query.
  static bool looks_like_url(const std::string &input);

  // Percent-encode a search query for use in a URL.
  static std::string encode_search_query(const std::string &query);
};

} // namespace open_browser
