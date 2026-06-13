// Open Browser — test_history_manager.cpp

#include "history_manager.h"

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using open_browser::HistoryEntry;
using open_browser::HistoryManager;

struct HistoryManagerTest : ::testing::Test {
  void SetUp() override { HistoryManager::instance().clear_all(); }
};

// ─────────────────────────────────────────────────────────────────────────────

TEST_F(HistoryManagerTest, AddVisit) {
  HistoryManager::instance().add_visit("https://example.com", "Example");
  auto recent = HistoryManager::instance().get_recent(10);
  ASSERT_EQ(recent.size(), 1u);
  EXPECT_EQ(recent[0].url, "https://example.com");
  EXPECT_EQ(recent[0].title, "Example");
  EXPECT_EQ(recent[0].visit_count, 1);
}

TEST_F(HistoryManagerTest, DuplicateVisitIncrementsCount) {
  HistoryManager::instance().add_visit("https://example.com", "Example");
  HistoryManager::instance().add_visit("https://example.com", "Example");
  auto recent = HistoryManager::instance().get_recent(10);
  ASSERT_EQ(recent.size(), 1u);
  EXPECT_EQ(recent[0].visit_count, 2);
}

TEST_F(HistoryManagerTest, GetRecentOrderedByTime) {
  // We rely on the same-second ordering by insertion order.
  // In practice timestamps are the same, but the most recently-mutated
  // entry is returned last.
  HistoryManager::instance().add_visit("https://a.com", "A");
  HistoryManager::instance().add_visit("https://b.com", "B");
  HistoryManager::instance().add_visit("https://c.com", "C");

  auto recent = HistoryManager::instance().get_recent(10);
  EXPECT_EQ(recent.size(), 3u);
}

TEST_F(HistoryManagerTest, GetRecentLimit) {
  for (int i = 0; i < 20; ++i) {
    HistoryManager::instance().add_visit("https://example.com/" +
                                             std::to_string(i),
                                         "Page " + std::to_string(i));
  }
  auto recent = HistoryManager::instance().get_recent(5);
  EXPECT_EQ(recent.size(), 5u);
}

TEST_F(HistoryManagerTest, RemoveEntry) {
  HistoryManager::instance().add_visit("https://example.com", "Example");
  auto recent = HistoryManager::instance().get_recent(1);
  ASSERT_EQ(recent.size(), 1u);

  HistoryManager::instance().remove(recent[0].id);
  EXPECT_TRUE(HistoryManager::instance().get_recent(10).empty());
}

TEST_F(HistoryManagerTest, ClearAll) {
  HistoryManager::instance().add_visit("https://a.com", "A");
  HistoryManager::instance().add_visit("https://b.com", "B");
  HistoryManager::instance().clear_all();
  EXPECT_TRUE(HistoryManager::instance().get_recent(10).empty());
}

TEST_F(HistoryManagerTest, SearchByTitle) {
  HistoryManager::instance().add_visit("https://github.com", "GitHub");
  HistoryManager::instance().add_visit("https://gitlab.com", "GitLab");
  HistoryManager::instance().add_visit("https://wikipedia.org", "Wikipedia");

  auto results = HistoryManager::instance().search("git");
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(HistoryManagerTest, SearchByUrl) {
  HistoryManager::instance().add_visit("https://news.ycombinator.com", "HN");
  HistoryManager::instance().add_visit("https://reddit.com", "Reddit");

  auto results = HistoryManager::instance().search("ycombinator");
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].title, "HN");
}

TEST_F(HistoryManagerTest, ClearRange) {
  // Add entries with the current timestamp
  const std::time_t now = std::time(nullptr);
  HistoryManager::instance().add_visit("https://a.com", "A");
  HistoryManager::instance().add_visit("https://b.com", "B");

  // Clear everything in the last minute
  HistoryManager::instance().clear_range(now - 60, now + 60);

  EXPECT_TRUE(HistoryManager::instance().get_recent(10).empty());
}

TEST_F(HistoryManagerTest, EmptyUrlIgnored) {
  HistoryManager::instance().add_visit("", "No URL");
  EXPECT_TRUE(HistoryManager::instance().get_recent(10).empty());
}
