// Open Browser — test_settings_manager.cpp

#include <gtest/gtest.h>

#include "settings_manager.h"

using open_browser::SettingsManager;

struct SettingsManagerTest : ::testing::Test {
  void SetUp() override {
    // Re-apply defaults by re-loading a fresh (non-existent) file.
    // We avoid touching the real config by directly testing the API.
    SettingsManager::instance().load();
  }
};

// ────────────────────────────────────────────────────────────────────

TEST_F(SettingsManagerTest, DefaultHomepage) {
  EXPECT_EQ(SettingsManager::instance().homepage(), "openbrowser://newtab");
}

TEST_F(SettingsManagerTest, DefaultSearchEngine) {
  const std::string se = SettingsManager::instance().search_engine();
  EXPECT_FALSE(se.empty());
  EXPECT_NE(se.find("?q="), std::string::npos);
}

TEST_F(SettingsManagerTest, DefaultFontSize) {
  const int fs = SettingsManager::instance().font_size();
  EXPECT_GT(fs, 0);
  EXPECT_LE(fs, 48);
}

TEST_F(SettingsManagerTest, DefaultBlockAds) {
  EXPECT_TRUE(SettingsManager::instance().block_ads());
}

TEST_F(SettingsManagerTest, DefaultBlockTrackers) {
  EXPECT_TRUE(SettingsManager::instance().block_trackers());
}

TEST_F(SettingsManagerTest, DefaultHardwareAcceleration) {
  EXPECT_TRUE(SettingsManager::instance().hardware_acceleration());
}

TEST_F(SettingsManagerTest, SetAndGetBool) {
  SettingsManager::instance().set<bool>("block_ads", false);
  EXPECT_FALSE(SettingsManager::instance().block_ads());
  // Restore
  SettingsManager::instance().set<bool>("block_ads", true);
}

TEST_F(SettingsManagerTest, SetAndGetInt) {
  SettingsManager::instance().set<int>("font_size", 18);
  EXPECT_EQ(SettingsManager::instance().font_size(), 18);
  // Restore
  SettingsManager::instance().set<int>("font_size", 16);
}

TEST_F(SettingsManagerTest, SetAndGetString) {
  const std::string custom = "https://duckduckgo.com/?q=";
  SettingsManager::instance().set<std::string>("search_engine", custom);
  EXPECT_EQ(SettingsManager::instance().search_engine(), custom);
  // Restore
  SettingsManager::instance().set<std::string>(
      "search_engine", "https://search.brave.com/search?q=");
}

TEST_F(SettingsManagerTest, ChangeCallback) {
  bool called = false;
  SettingsManager::instance().on_change(
      "font_size", [&](const open_browser::SettingValue &) { called = true; });
  SettingsManager::instance().set<int>("font_size", 20);
  EXPECT_TRUE(called);
  // Restore
  SettingsManager::instance().set<int>("font_size", 16);
}

TEST_F(SettingsManagerTest, GetMissingKeyReturnsDefault) {
  const std::string val =
      SettingsManager::instance().get<std::string>("nonexistent_key_xyz");
  EXPECT_TRUE(val.empty());
}

TEST_F(SettingsManagerTest, ThemeDefault) {
  const std::string theme = SettingsManager::instance().theme();
  EXPECT_FALSE(theme.empty());
  // Should be one of system/light/dark
  EXPECT_TRUE(theme == "system" || theme == "light" || theme == "dark");
}

TEST_F(SettingsManagerTest, DownloadPathNonEmpty) {
  const std::string dp = SettingsManager::instance().download_path();
  EXPECT_FALSE(dp.empty());
}
