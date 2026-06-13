// Open Browser — password_manager.cpp
// Password management with libsecret keyring integration.

#include "password_manager.h"

#include <glib.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <libsecret/secret.h>
#include <stdexcept>

namespace open_browser {

using json = nlohmann::json;

// libsecret schema for Open Browser passwords
static const SecretSchema kPasswordSchema = {
    "io.openbrowser.Password",
    SECRET_SCHEMA_NONE,
    {{"origin", SECRET_SCHEMA_ATTRIBUTE_STRING},
     {"username", SECRET_SCHEMA_ATTRIBUTE_STRING},
     {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}},
    0, // reserved
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr // reserved1-7
};

// ─────────────────────────────────────────────────────────────────────────────

PasswordManager &PasswordManager::instance() {
  static PasswordManager inst;
  return inst;
}

void PasswordManager::initialize(const std::string &master_key) {
  if (initialized_)
    return;

  encryption_key_ = master_key.empty()
                        ? std::string("open-browser-default-key-2024")
                        : master_key;

  load();
  initialized_ = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Persistence (metadata only — secrets go to keyring)
// ─────────────────────────────────────────────────────────────────────────────

void PasswordManager::load() {
  const std::string path = get_data_path();
  if (!std::filesystem::exists(path))
    return;

  try {
    std::ifstream f(path);
    json j = json::parse(f);

    if (j.contains("entries") && j["entries"].is_array()) {
      for (const auto &item : j["entries"]) {
        PasswordEntry entry;
        entry.id = item.value("id", int64_t{0});
        entry.origin = item.value("origin", std::string{});
        entry.username = item.value("username", std::string{});
        entry.display_name = item.value("display_name", std::string{});
        entry.created_at = item.value("created_at", std::time_t{0});
        entry.last_used = item.value("last_used", std::time_t{0});
        // encrypted_password stays in keyring; not stored in JSON
        if (entry.id >= next_id_)
          next_id_ = entry.id + 1;
        entries_.push_back(std::move(entry));
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "[PasswordManager] Load error: " << e.what() << "\n";
  }
}

void PasswordManager::save() const {
  const std::string path = get_data_path();
  try {
    std::filesystem::create_directories(
        std::filesystem::path(path).parent_path());

    json j;
    j["entries"] = json::array();
    for (const auto &entry : entries_) {
      j["entries"].push_back({{"id", entry.id},
                              {"origin", entry.origin},
                              {"username", entry.username},
                              {"display_name", entry.display_name},
                              {"created_at", entry.created_at},
                              {"last_used", entry.last_used}});
    }

    std::ofstream f(path);
    f << j.dump(2) << "\n";
  } catch (const std::exception &e) {
    std::cerr << "[PasswordManager] Save error: " << e.what() << "\n";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// CRUD
// ─────────────────────────────────────────────────────────────────────────────

bool PasswordManager::save_password(const std::string &origin,
                                    const std::string &username,
                                    const std::string &password) {
  if (!initialized_)
    initialize();

  // Remove existing entry for the same origin+username
  auto it = std::find_if(entries_.begin(), entries_.end(),
                         [&](const PasswordEntry &e) {
                           return e.origin == origin && e.username == username;
                         });

  const std::string label = make_secret_label(origin, username);

  if (it != entries_.end()) {
    // Update existing
    it->last_used = std::time(nullptr);
    store_in_keyring(label, password);
    save();
    return true;
  }

  // New entry
  PasswordEntry entry;
  entry.id = next_id_++;
  entry.origin = origin;
  entry.username = username;
  entry.display_name = username + " @ " + origin;
  entry.created_at = std::time(nullptr);
  entry.last_used = entry.created_at;

  if (!store_in_keyring(label, password)) {
    // Fall back to obfuscated storage in the metadata file
    entry.encrypted_password = xor_obfuscate(password, encryption_key_);
  }

  entries_.push_back(std::move(entry));
  save();
  return true;
}

std::optional<std::string>
PasswordManager::get_password(const std::string &origin,
                              const std::string &username) {
  if (!initialized_)
    initialize();

  auto it = std::find_if(entries_.begin(), entries_.end(),
                         [&](const PasswordEntry &e) {
                           return e.origin == origin && e.username == username;
                         });
  if (it == entries_.end())
    return std::nullopt;

  it->last_used = std::time(nullptr);
  save();

  const std::string label = make_secret_label(origin, username);
  auto secret = retrieve_from_keyring(label);
  if (secret)
    return secret;

  // Fall back to in-file obfuscated
  if (!it->encrypted_password.empty()) {
    return xor_obfuscate(it->encrypted_password, encryption_key_);
  }
  return std::nullopt;
}

std::vector<PasswordEntry>
PasswordManager::get_for_origin(const std::string &origin) const {
  std::vector<PasswordEntry> result;
  for (const auto &e : entries_) {
    if (e.origin == origin)
      result.push_back(e);
  }
  return result;
}

std::vector<PasswordEntry> PasswordManager::get_all() const {
  return entries_;
}

bool PasswordManager::remove(int64_t id) {
  auto it = std::find_if(entries_.begin(), entries_.end(),
                         [id](const PasswordEntry &e) { return e.id == id; });
  if (it == entries_.end())
    return false;

  const std::string label = make_secret_label(it->origin, it->username);
  delete_from_keyring(label);
  entries_.erase(it);
  save();
  return true;
}

bool PasswordManager::update_password(int64_t id,
                                      const std::string &new_password) {
  auto it = std::find_if(entries_.begin(), entries_.end(),
                         [id](const PasswordEntry &e) { return e.id == id; });
  if (it == entries_.end())
    return false;

  const std::string label = make_secret_label(it->origin, it->username);
  return store_in_keyring(label, new_password);
}

std::vector<PasswordEntry>
PasswordManager::find_matching(const std::string &url) const {
  // Extract the host from the URL for matching
  std::string host = url;
  for (const auto &scheme : {"https://", "http://"}) {
    if (host.starts_with(scheme)) {
      host = host.substr(std::string(scheme).size());
      break;
    }
  }
  // Strip path
  const size_t slash = host.find('/');
  if (slash != std::string::npos)
    host = host.substr(0, slash);

  std::vector<PasswordEntry> result;
  for (const auto &e : entries_) {
    std::string origin_host = e.origin;
    for (const auto &scheme : {"https://", "http://"}) {
      if (origin_host.starts_with(scheme)) {
        origin_host = origin_host.substr(std::string(scheme).size());
        break;
      }
    }
    if (host == origin_host || host.ends_with("." + origin_host)) {
      result.push_back(e);
    }
  }
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Keyring helpers
// ─────────────────────────────────────────────────────────────────────────────

bool PasswordManager::store_in_keyring(const std::string &label,
                                       const std::string &secret) {
  GError *error = nullptr;
  gboolean result = secret_password_store_sync(
      &kPasswordSchema, SECRET_COLLECTION_DEFAULT, label.c_str(),
      secret.c_str(),
      nullptr, // cancellable
      &error, "origin", label.c_str(), "username", label.c_str(), nullptr);

  if (error) {
    std::cerr << "[PasswordManager] Keyring store error: " << error->message
              << "\n";
    g_error_free(error);
    return false;
  }
  return result == TRUE;
}

std::optional<std::string>
PasswordManager::retrieve_from_keyring(const std::string &label) {
  GError *error = nullptr;
  gchar *secret = secret_password_lookup_sync(
      &kPasswordSchema,
      nullptr, // cancellable
      &error, "origin", label.c_str(), "username", label.c_str(), nullptr);

  if (error) {
    g_error_free(error);
    return std::nullopt;
  }
  if (!secret)
    return std::nullopt;

  std::string result(secret);
  secret_password_free(secret);
  return result;
}

void PasswordManager::delete_from_keyring(const std::string &label) {
  GError *error = nullptr;
  secret_password_clear_sync(&kPasswordSchema, nullptr, &error, "origin",
                             label.c_str(), "username", label.c_str(), nullptr);
  if (error) {
    g_error_free(error);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Utilities
// ─────────────────────────────────────────────────────────────────────────────

std::string
PasswordManager::make_secret_label(const std::string &origin,
                                   const std::string &username) const {
  return "open-browser:" + origin + ":" + username;
}

std::string PasswordManager::xor_obfuscate(const std::string &text,
                                           const std::string &key) const {
  if (key.empty())
    return text;
  std::string result = text;
  for (size_t i = 0; i < result.size(); ++i) {
    result[i] ^= key[i % key.size()];
  }
  return result;
}

std::string PasswordManager::get_data_path() const {
  const char *data_dir = g_get_user_data_dir();
  const std::string base =
      data_dir ? data_dir : (std::string(g_get_home_dir()) + "/.local/share");
  return base + "/open-browser/passwords.json";
}

} // namespace open_browser
