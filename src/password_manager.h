#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <vector>

namespace open_browser {

struct PasswordEntry {
    int64_t     id                 = 0;
    std::string origin;
    std::string username;
    std::string encrypted_password; // XOR-obfuscated; real impl uses libsecret
    std::string display_name;
    std::time_t created_at         = 0;
    std::time_t last_used          = 0;
};

// Singleton password manager.
// Stores credentials in ~/.local/share/open-browser/passwords.json with
// the raw password protected via the system secret service (libsecret).
// The JSON file only stores non-sensitive metadata; secrets are stored
// separately in the keyring.
class PasswordManager {
public:
    static PasswordManager& instance();

    // Initialize and optionally set an application-level master key.
    void initialize(const std::string& master_key = "");

    bool save_password(const std::string& origin,
                       const std::string& username,
                       const std::string& password);

    std::optional<std::string> get_password(const std::string& origin,
                                             const std::string& username);

    std::vector<PasswordEntry> get_for_origin(const std::string& origin) const;
    std::vector<PasswordEntry> get_all() const;
    bool remove(int64_t id);
    bool update_password(int64_t id, const std::string& new_password);

    // Auto-fill: find all entries whose origin matches or is a suffix of `url`.
    std::vector<PasswordEntry> find_matching(const std::string& url) const;

private:
    PasswordManager() = default;
    PasswordManager(const PasswordManager&) = delete;
    PasswordManager& operator=(const PasswordManager&) = delete;

    void load();
    void save() const;

    std::string get_data_path() const;
    std::string make_secret_label(const std::string& origin,
                                   const std::string& username) const;

    // Keyring helpers (libsecret)
    bool store_in_keyring(const std::string& label,
                          const std::string& secret);
    std::optional<std::string> retrieve_from_keyring(const std::string& label);
    void delete_from_keyring(const std::string& label);

    // Fallback encryption for systems without a secret service
    std::string xor_obfuscate(const std::string& text,
                               const std::string& key) const;

    std::vector<PasswordEntry> entries_;
    std::string encryption_key_;
    int64_t next_id_ = 1;
    bool initialized_ = false;
};

} // namespace open_browser
