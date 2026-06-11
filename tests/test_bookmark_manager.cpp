// Open Browser — test_bookmark_manager.cpp

#include <gtest/gtest.h>
#include "bookmark_manager.h"

using open_browser::Bookmark;
using open_browser::BookmarkFolder;
using open_browser::BookmarkManager;

// Helper: clear all bookmarks between tests
struct BookmarkManagerTest : ::testing::Test {
    void SetUp() override {
        // Clear by removing all entries through the public API
        auto& mgr = BookmarkManager::instance();
        for (const auto& bm : mgr.get_all()) mgr.remove(bm.id);
        for (const auto& f  : mgr.get_folders()) mgr.remove_folder(f.id);
    }
};

// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BookmarkManagerTest, AddAndRetrieve) {
    auto& mgr = BookmarkManager::instance();
    Bookmark bm = mgr.add("https://example.com", "Example");

    EXPECT_GT(bm.id, 0);
    EXPECT_EQ(bm.url,   "https://example.com");
    EXPECT_EQ(bm.title, "Example");

    auto all = mgr.get_all();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].url, "https://example.com");
}

TEST_F(BookmarkManagerTest, Remove) {
    auto& mgr = BookmarkManager::instance();
    Bookmark bm = mgr.add("https://example.com", "Example");
    EXPECT_TRUE(mgr.remove(bm.id));
    EXPECT_TRUE(mgr.get_all().empty());
}

TEST_F(BookmarkManagerTest, RemoveNonexistent) {
    auto& mgr = BookmarkManager::instance();
    EXPECT_FALSE(mgr.remove(999));
}

TEST_F(BookmarkManagerTest, Update) {
    auto& mgr = BookmarkManager::instance();
    Bookmark bm = mgr.add("https://example.com", "Old title");
    EXPECT_TRUE(mgr.update(bm.id, "New title", "work"));

    auto updated = mgr.find_by_url("https://example.com");
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->title,  "New title");
    EXPECT_EQ(updated->folder, "work");
}

TEST_F(BookmarkManagerTest, IsBookmarked) {
    auto& mgr = BookmarkManager::instance();
    EXPECT_FALSE(mgr.is_bookmarked("https://example.com"));
    mgr.add("https://example.com", "Test");
    EXPECT_TRUE(mgr.is_bookmarked("https://example.com"));
}

TEST_F(BookmarkManagerTest, SearchByTitle) {
    auto& mgr = BookmarkManager::instance();
    mgr.add("https://github.com",    "GitHub");
    mgr.add("https://gitlab.com",    "GitLab");
    mgr.add("https://wikipedia.org", "Wikipedia");

    auto results = mgr.search("git");
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(BookmarkManagerTest, SearchByUrl) {
    auto& mgr = BookmarkManager::instance();
    mgr.add("https://news.ycombinator.com", "Hacker News");
    mgr.add("https://reddit.com",           "Reddit");

    auto results = mgr.search("ycombinator");
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].title, "Hacker News");
}

TEST_F(BookmarkManagerTest, GetByFolder) {
    auto& mgr = BookmarkManager::instance();
    mgr.add("https://example.com",  "Work site",  "work");
    mgr.add("https://youtube.com",  "YouTube",    "personal");
    mgr.add("https://github.com",   "GitHub",     "work");

    auto work = mgr.get_by_folder("work");
    EXPECT_EQ(work.size(), 2u);

    auto personal = mgr.get_by_folder("personal");
    EXPECT_EQ(personal.size(), 1u);
}

TEST_F(BookmarkManagerTest, FindByUrlNone) {
    auto& mgr = BookmarkManager::instance();
    auto result = mgr.find_by_url("https://nonexistent.example");
    EXPECT_FALSE(result.has_value());
}

TEST_F(BookmarkManagerTest, AddAndRemoveFolder) {
    auto& mgr = BookmarkManager::instance();
    BookmarkFolder f = mgr.add_folder("Work", 0);
    EXPECT_GT(f.id, 0);
    EXPECT_EQ(f.name, "Work");

    auto folders = mgr.get_folders();
    ASSERT_EQ(folders.size(), 1u);

    EXPECT_TRUE(mgr.remove_folder(f.id));
    EXPECT_TRUE(mgr.get_folders().empty());
}

TEST_F(BookmarkManagerTest, TitleFallsBackToUrl) {
    auto& mgr = BookmarkManager::instance();
    Bookmark bm = mgr.add("https://example.com", "");
    EXPECT_EQ(bm.title, "https://example.com");
}
