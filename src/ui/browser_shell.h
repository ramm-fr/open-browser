#pragma once

#include "address_bar.h"
#include "navigation_bar.h"
#include "tab_strip.h"
#include "window_controls.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>

namespace open_browser::ui {

// BrowserShell — assembles the complete browser chrome.
//
// New layout:
//   Row 1: [TabStrip (left)] [WindowControls (far right)]
//   Row 2: [(←)(→)(⟳)] [────── Address Bar ──────] [(≡)]
//   Row 3: [GtkStack — web content]
//
// The title bar is a minimal GtkHeaderBar used only for window drag.
// All visible chrome is in custom GTK boxes below it.
class BrowserShell {
public:
  struct Callbacks {
    // Navigation
    std::function<void()> on_back;
    std::function<void()> on_forward;
    std::function<void()> on_reload_or_stop;
    std::function<void(const std::string &)> on_navigate;

    // Tabs
    std::function<void(int)> on_tab_clicked;
    std::function<void(int)> on_tab_closed;
    std::function<void()> on_new_tab;

    // Window controls
    std::function<void()> on_minimize;
    std::function<void()> on_maximize;
    std::function<void()> on_close;
  };

  explicit BrowserShell(GtkWindow *window);
  ~BrowserShell();

  // Wire all callbacks. Must be called before show().
  void set_callbacks(const Callbacks &callbacks);

  // ── Widget access ──────────────────────────────────────────────────────
  GtkWidget *get_root_vbox() const { return main_vbox_; }
  GtkWidget *get_content_stack() const { return content_stack_; }
  GtkWidget *get_header_bar() const { return header_bar_; }
  GtkWidget *get_menu_button() const { return menu_button_; }

  // ── Address bar operations ─────────────────────────────────────────────
  void set_url(const std::string &url);
  void set_secure(bool secure);
  void focus_address_bar();

  // ── Navigation bar operations ──────────────────────────────────────────
  void set_can_go_back(bool can);
  void set_can_go_forward(bool can);
  void set_loading(bool loading);

  // ── Tab strip operations ───────────────────────────────────────────────
  void clear_tabs();
  void add_tab(int tab_id, const std::string &title, bool loading, bool active);

private:
  void build_ui(GtkWindow *window);

  // Root container
  GtkWidget *main_vbox_ = nullptr;

  // Minimal header bar (for window drag only)
  GtkWidget *header_bar_ = nullptr;

  // Row 1 widgets
  GtkWidget *tab_row_ = nullptr;
  GtkWidget *menu_button_ = nullptr;

  // Content area
  GtkWidget *content_stack_ = nullptr;

  // Modular components
  std::unique_ptr<NavigationBar> nav_bar_;
  std::unique_ptr<AddressBar> address_bar_;
  std::unique_ptr<TabStrip> tab_strip_;
  std::unique_ptr<WindowControls> window_controls_;
};

} // namespace open_browser::ui
