#pragma once

#include <gtk/gtk.h>

#include <functional>
#include <string>

namespace open_browser::ui {

// Reusable address bar component with security icon and URL entry.
// Produces the same widget structure as the original inline implementation:
// [HBox: [security-icon] [entry#address-bar]]
class AddressBar {
public:
  using NavigateCallback = std::function<void(const std::string &url)>;

  AddressBar();
  ~AddressBar();

  // Get the root widget (HBox container) for packing into the header bar
  GtkWidget *get_widget() const { return container_; }

  // Set the callback fired when the user presses Enter
  void set_navigate_callback(NavigateCallback callback) {
    on_navigate_ = std::move(callback);
  }

  // Update the displayed URL text
  void set_url(const std::string &url);

  // Update the security indicator based on the URL scheme
  void set_secure(bool secure);

  // Grab focus and select all text (Ctrl+L behavior)
  void focus_and_select_all();

private:
  GtkWidget *container_ = nullptr;
  GtkWidget *security_icon_ = nullptr;
  GtkWidget *entry_ = nullptr;

  NavigateCallback on_navigate_;

  void build_ui();

  // Static signal callback
  static void on_activate(GtkEntry *entry, gpointer user_data);
};

} // namespace open_browser::ui
