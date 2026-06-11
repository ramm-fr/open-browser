// Open Browser — test_ad_blocker.cpp

#include <gtest/gtest.h>
#include "ad_blocker.h"

using open_browser::AdBlocker;

struct AdBlockerTest : ::testing::Test {
    void SetUp() override {
        AdBlocker::instance().reset_count();
        // Load built-in lists (safe to call multiple times)
        AdBlocker::instance().load_filter_lists();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Tracker blocking
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdBlockerTest, BlocksGoogleAnalytics) {
    EXPECT_TRUE(AdBlocker::instance().should_block(
        "https://www.google-analytics.com/analytics.js", ""));
}

TEST_F(AdBlockerTest, BlocksDoubleclick) {
    EXPECT_TRUE(AdBlocker::instance().should_block(
        "https://stats.g.doubleclick.net/j/collect", ""));
}

TEST_F(AdBlockerTest, BlocksFacebook) {
    EXPECT_TRUE(AdBlocker::instance().should_block(
        "https://connect.facebook.net/en_US/fbevents.js", ""));
}

TEST_F(AdBlockerTest, BlocksHotjar) {
    EXPECT_TRUE(AdBlocker::instance().should_block(
        "https://static.hotjar.com/c/hotjar-123.js", ""));
}

TEST_F(AdBlockerTest, BlocksCriteo) {
    EXPECT_TRUE(AdBlocker::instance().should_block(
        "https://static.criteo.net/js/ld/publishertag.js", ""));
}

// ─────────────────────────────────────────────────────────────────────────────
// Legitimate traffic not blocked
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdBlockerTest, DoesNotBlockGitHub) {
    EXPECT_FALSE(AdBlocker::instance().should_block(
        "https://github.com/user/repo", ""));
}

TEST_F(AdBlockerTest, DoesNotBlockWikipedia) {
    EXPECT_FALSE(AdBlocker::instance().should_block(
        "https://en.wikipedia.org/wiki/Linux", ""));
}

TEST_F(AdBlockerTest, DoesNotBlockCDNjs) {
    EXPECT_FALSE(AdBlocker::instance().should_block(
        "https://cdnjs.cloudflare.com/ajax/libs/lodash.js/4.17.21/lodash.min.js", ""));
}

// ─────────────────────────────────────────────────────────────────────────────
// Subdomain blocking
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdBlockerTest, BlocksSubdomainOfTrackerDomain) {
    EXPECT_TRUE(AdBlocker::instance().should_block(
        "https://pixel.quantserve.com/pixel/abc123", ""));
}

// ─────────────────────────────────────────────────────────────────────────────
// Whitelist
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdBlockerTest, WhitelistExemptsFromBlocking) {
    // This would normally be blocked
    const std::string url = "https://hotjar.com/c/hotjar.js";
    EXPECT_TRUE(AdBlocker::instance().should_block(url, ""));

    // Now whitelist the domain
    AdBlocker::instance().add_whitelist_domain("hotjar.com");
    EXPECT_FALSE(AdBlocker::instance().should_block(url, ""));

    // Clean up
    AdBlocker::instance().remove_whitelist_domain("hotjar.com");
}

TEST_F(AdBlockerTest, IsWhitelistedCheck) {
    AdBlocker::instance().add_whitelist_domain("mysite.com");
    EXPECT_TRUE(AdBlocker::instance().is_whitelisted("mysite.com"));
    EXPECT_FALSE(AdBlocker::instance().is_whitelisted("othersite.com"));
    AdBlocker::instance().remove_whitelist_domain("mysite.com");
}

// ─────────────────────────────────────────────────────────────────────────────
// Count tracking
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdBlockerTest, BlockedCountIncremented) {
    const size_t before = AdBlocker::instance().blocked_count();
    AdBlocker::instance().should_block("https://www.google-analytics.com/collect", "");
    const size_t after  = AdBlocker::instance().blocked_count();
    EXPECT_GT(after, before);
}

TEST_F(AdBlockerTest, ResetCount) {
    AdBlocker::instance().should_block("https://www.google-analytics.com/collect", "");
    EXPECT_GT(AdBlocker::instance().blocked_count(), 0u);
    AdBlocker::instance().reset_count();
    EXPECT_EQ(AdBlocker::instance().blocked_count(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tracker-specific check
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdBlockerTest, ShouldBlockTrackerForKnownTracker) {
    EXPECT_TRUE(AdBlocker::instance().should_block_tracker(
        "https://mc.yandex.ru/metrika/tag.js"));
}

TEST_F(AdBlockerTest, ShouldNotBlockTrackerForLegitSite) {
    EXPECT_FALSE(AdBlocker::instance().should_block_tracker(
        "https://github.com/user/repo"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge cases
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdBlockerTest, EmptyUrlNotBlocked) {
    EXPECT_FALSE(AdBlocker::instance().should_block("", ""));
}

TEST_F(AdBlockerTest, MalformedUrlNotBlocked) {
    EXPECT_FALSE(AdBlocker::instance().should_block("not-a-url", ""));
}
