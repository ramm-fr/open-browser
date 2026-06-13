#pragma once

#include <gtk/gtk.h>

#include <functional>
#include <string>

namespace open_browser::ui {

// Tab strip component — manages the horizontal tab bar with scrollable tabs
// and a new-tab button. Visual output matches the existing BrowserWindow
// tab bar exactly: [scrolled: [tab-outer[tab-btn][close-btn]]...] [+ button]
class TabStrip {
public:
  using TabCallback = std::function<void(int tab_id)>;
  using NewTabCallback = std::function<void()>;

  TabStrip();
  ~TabStrip();

  // Get the root widget to pack into the window layout
  GtkWidget *get_widget() const { return container_; }

  // Full rebuild — removes all tabs and recreates from data provided via
  // repeated add_tab() calls. Call clear() first, then add_tab() for each tab.
  void clear();
  void add_tab(int tab_id, const std::string &title, bool loading, bool active);

  // Set callbacks
  void set_tab_clicked_callback(TabCallback callback) {
    on_tab_clicked_ = std::move(callback);
  }
  void set_tab_closed_callback(TabCallback callback) {
    on_tab_closed_ = std::move(callback);
  }
  void set_new_tab_callback(NewTabCallback callback) {
    on_new_tab_ = std::move(callback);
  }

private:
  GtkWidget *container_ = nullptr;   // Outer HBox: [scroll] [+ btn]
  GtkWidget *scroll_ = nullptr;      // ScrolledWindow for tabs
  GtkWidget *tabs_box_ = nullptr;    // HBox inside scroll holding tab widgets
  GtkWidget *new_tab_btn_ = nullptr; // The "+" button

  TabCallback on_tab_clicked_;
  TabCallback on_tab_closed_;
  NewTabCallback on_new_tab_;

  void build_ui();

  // Static signal callbacks
  static void on_new_tab_button_clicked(GtkButton *button, gpointer user_data);
};

} // namespace open_browser::ui
