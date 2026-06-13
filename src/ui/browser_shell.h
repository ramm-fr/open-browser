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

// BrowserShell — a reusable container that assembles the complete browser
// chrome: header bar (navigation + address bar + menu + window controls),
// tab strip, and a content area (GtkStack for WebViews).
//
// Layout (top to bottom):
//   [GtkHeaderBar: [NavBar] [AddressBar (title widget)] [+NewTab] [Menu]
//   [WinCtrl]] [TabStrip: scrollable tabs + new-tab button] [GtkStack: one page
//   per tab (content area)]
//
// BrowserShell does NOT own WebKitWebViews or manage tab state.
// It provides widget containers and callbacks — the owner (BrowserWindow)
// wires up the logic.
class BrowserShell {
public:
  // Callbacks the owner sets to respond to user interactions
  struct Callbacks {
    // Navigation
    std::function<void()> on_back;
    std::function<void()> on_forward;
    std::function<void()> on_reload_or_stop;
    std::function<void(const std::string &)> on_navigate; // address bar Enter

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

  // ── Setup ──────────────────────────────────────────────────────────────

  // Wire all callbacks. Must be called before show().
  void set_callbacks(const Callbacks &callbacks);

  // ── Widget access ──────────────────────────────────────────────────────

  // The root vertical box (window child). Contains header bar, tab strip,
  // and content stack from top to bottom.
  GtkWidget *get_root_vbox() const { return main_vbox_; }

  // The GtkStack where WebView pages are added by the owner.
  GtkWidget *get_content_stack() const { return content_stack_; }

  // The GtkHeaderBar (set as window titlebar by BrowserShell).
  GtkWidget *get_header_bar() const { return header_bar_; }

  // ── Address bar operations ─────────────────────────────────────────────

  void set_url(const std::string &url);
  void set_secure(bool secure);
  void focus_address_bar();

  // ── Navigation bar operations ──────────────────────────────────────────

  void set_can_go_back(bool can);
  void set_can_go_forward(bool can);
  void set_loading(bool loading);

  // ── Tab strip operations ───────────────────────────────────────────────

  // Full rebuild: clear all tabs, then add each one.
  void clear_tabs();
  void add_tab(int tab_id, const std::string &title, bool loading, bool active);

  // ── Menu button access (for owner to attach GMenuModel) ────────────────

  GtkWidget *get_menu_button() const { return menu_button_; }

private:
  void build_ui(GtkWindow *window);

  // Root container
  GtkWidget *main_vbox_ = nullptr;

  // Header bar and its children
  GtkWidget *header_bar_ = nullptr;
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
