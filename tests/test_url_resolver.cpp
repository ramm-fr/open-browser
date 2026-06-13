// Open Browser — test_url_resolver.cpp

#include "url_resolver.h"

#include <gtest/gtest.h>

using open_browser::UrlResolver;

// ─────────────────────────────────────────────────────────────────────────────
// looks_like_url
// ─────────────────────────────────────────────────────────────────────────────

TEST(UrlResolverLooksLikeUrl, DomainWithTLD) {
  EXPECT_TRUE(UrlResolver::looks_like_url("google.com"));
  EXPECT_TRUE(UrlResolver::looks_like_url("example.org"));
  EXPECT_TRUE(UrlResolver::looks_like_url("sub.domain.co.uk"));
}

TEST(UrlResolverLooksLikeUrl, DomainWithPath) {
  EXPECT_TRUE(UrlResolver::looks_like_url("github.com/user/repo"));
  EXPECT_TRUE(UrlResolver::looks_like_url("en.wikipedia.org/wiki/Linux"));
}

TEST(UrlResolverLooksLikeUrl, Localhost) {
  EXPECT_TRUE(UrlResolver::looks_like_url("localhost"));
  EXPECT_TRUE(UrlResolver::looks_like_url("localhost:8080"));
  EXPECT_TRUE(UrlResolver::looks_like_url("127.0.0.1:3000"));
}

TEST(UrlResolverLooksLikeUrl, WithScheme) {
  EXPECT_TRUE(UrlResolver::looks_like_url("https://example.com"));
  EXPECT_TRUE(UrlResolver::looks_like_url("http://example.com"));
  EXPECT_TRUE(UrlResolver::looks_like_url("ftp://ftp.example.com"));
}

TEST(UrlResolverLooksLikeUrl, SearchQueries) {
  EXPECT_FALSE(UrlResolver::looks_like_url("hello world"));
  EXPECT_FALSE(UrlResolver::looks_like_url("what is linux"));
  EXPECT_FALSE(UrlResolver::looks_like_url("open source software"));
}

TEST(UrlResolverLooksLikeUrl, SingleWord) {
  // Single words without a dot should NOT look like a URL
  EXPECT_FALSE(UrlResolver::looks_like_url("linux"));
  EXPECT_FALSE(UrlResolver::looks_like_url("google"));
}

TEST(UrlResolverLooksLikeUrl, EmptyString) {
  EXPECT_FALSE(UrlResolver::looks_like_url(""));
}

// ─────────────────────────────────────────────────────────────────────────────
// resolve
// ─────────────────────────────────────────────────────────────────────────────

TEST(UrlResolverResolve, BareDomainsGetHttps) {
  const std::string url = UrlResolver::resolve("google.com");
  EXPECT_EQ(url, "https://google.com");
}

TEST(UrlResolverResolve, HttpSchemePreserved) {
  const std::string url = UrlResolver::resolve("http://example.com");
  EXPECT_EQ(url, "http://example.com");
}

TEST(UrlResolverResolve, HttpsSchemePreserved) {
  const std::string url = UrlResolver::resolve("https://example.com/path?q=1");
  EXPECT_EQ(url, "https://example.com/path?q=1");
}

TEST(UrlResolverResolve, SearchQueryEncodesCorrectly) {
  const std::string url =
      UrlResolver::resolve("what is C++", "https://search.brave.com/search?q=");
  EXPECT_TRUE(url.starts_with("https://search.brave.com/search?q="));
  EXPECT_TRUE(url.find("C%2B%2B") != std::string::npos ||
              url.find("C++") != std::string::npos);
}

TEST(UrlResolverResolve, InternalUrlPassedThrough) {
  const std::string url = UrlResolver::resolve("openbrowser://newtab");
  EXPECT_EQ(url, "openbrowser://newtab");
}

TEST(UrlResolverResolve, WhitespaceStripped) {
  const std::string url = UrlResolver::resolve("  google.com  ");
  EXPECT_EQ(url, "https://google.com");
}

TEST(UrlResolverResolve, FileSchemePreserved) {
  const std::string url = UrlResolver::resolve("file:///home/user/page.html");
  EXPECT_EQ(url, "file:///home/user/page.html");
}

// ─────────────────────────────────────────────────────────────────────────────
// is_internal_url
// ─────────────────────────────────────────────────────────────────────────────

TEST(UrlResolverInternal, RecognisesInternalScheme) {
  EXPECT_TRUE(UrlResolver::is_internal_url("openbrowser://newtab"));
  EXPECT_TRUE(UrlResolver::is_internal_url("openbrowser://settings"));
  EXPECT_TRUE(UrlResolver::is_internal_url("openbrowser://history"));
}

TEST(UrlResolverInternal, ExternalUrlsNotInternal) {
  EXPECT_FALSE(UrlResolver::is_internal_url("https://example.com"));
  EXPECT_FALSE(UrlResolver::is_internal_url("http://example.com"));
  EXPECT_FALSE(UrlResolver::is_internal_url("about:blank"));
}

// ─────────────────────────────────────────────────────────────────────────────
// encode_search_query
// ─────────────────────────────────────────────────────────────────────────────

TEST(UrlResolverEncode, SpacesEncodedAsPlus) {
  const std::string encoded = UrlResolver::encode_search_query("hello world");
  EXPECT_EQ(encoded, "hello+world");
}

TEST(UrlResolverEncode, AlphanumericUnchanged) {
  const std::string encoded =
      UrlResolver::encode_search_query("OpenBrowser123");
  EXPECT_EQ(encoded, "OpenBrowser123");
}

TEST(UrlResolverEncode, SpecialCharsEncoded) {
  const std::string encoded = UrlResolver::encode_search_query("a&b=c");
  EXPECT_NE(encoded.find('%'), std::string::npos);
}

TEST(UrlResolverEncode, EmptyString) {
  EXPECT_EQ(UrlResolver::encode_search_query(""), "");
}
