#include "browser_shell.h"

#include <glib.h>

namespace open_browser::ui {

BrowserShell::BrowserShell(GtkWindow *window) {
  build_ui(window);
}

BrowserShell::~BrowserShell() {}

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────

void BrowserShell::set_callbacks(const Callbacks &cb) {
  nav_bar_->set_back_callback(cb.on_back);
  nav_bar_->set_forward_callback(cb.on_forward);
  nav_bar_->set_reload_or_stop_callback(cb.on_reload_or_stop);
  address_bar_->set_navigate_callback(cb.on_navigate);
  tab_strip_->set_tab_clicked_callback(cb.on_tab_clicked);
  tab_strip_->set_tab_closed_callback(cb.on_tab_closed);
  tab_strip_->set_new_tab_callback(cb.on_new_tab);
  window_controls_->set_minimize_callback(cb.on_minimize);
  window_controls_->set_maximize_callback(cb.on_maximize);
  window_controls_->set_close_callback(cb.on_close);
}

// ─────────────────────────────────────────────────────────────────────────────
// Delegations
// ─────────────────────────────────────────────────────────────────────────────

void BrowserShell::set_url(const std::string &url) {
  address_bar_->set_url(url);
}

void BrowserShell::set_secure(bool secure) {
  address_bar_->set_secure(secure);
}

void BrowserShell::focus_address_bar() {
  address_bar_->focus_and_select_all();
}

void BrowserShell::set_can_go_back(bool can) {
  nav_bar_->set_can_go_back(can);
}

void BrowserShell::set_can_go_forward(bool can) {
  nav_bar_->set_can_go_forward(can);
}

void BrowserShell::set_loading(bool loading) {
  nav_bar_->set_loading(loading);
}

void BrowserShell::clear_tabs() {
  tab_strip_->clear();
}

void BrowserShell::add_tab(int tab_id, const std::string &title, bool loading,
                           bool active) {
  tab_strip_->add_tab(tab_id, title, loading, active);
}

// ─────────────────────────────────────────────────────────────────────────────
// UI Construction
//
// Layout:
//   [GtkHeaderBar — invisible, zero height, only for window drag/CSD]
//   [Row 1: tabs (left, scrollable) | window controls (right)]
//   [Row 2: nav buttons | address bar (centered, expands) | menu]
//   [Row 3: content stack]
// ─────────────────────────────────────────────────────────────────────────────

void BrowserShell::build_ui(GtkWindow *window) {
  // Create components
  nav_bar_ = std::make_unique<NavigationBar>();
  address_bar_ = std::make_unique<AddressBar>();
  tab_strip_ = std::make_unique<TabStrip>();
  window_controls_ = std::make_unique<WindowControls>();

  // ── Invisible header bar for CSD window drag ───────────────────────────
  header_bar_ = gtk_header_bar_new();
  gtk_widget_set_name(header_bar_, "shell-titlebar");
  gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header_bar_), FALSE);
  gtk_window_set_titlebar(window, header_bar_);

  // Make it effectively invisible — zero padding, minimal height
  // The actual chrome is drawn in main_vbox_ below

  // ── Root VBox ──────────────────────────────────────────────────────────
  main_vbox_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_name(main_vbox_, "shell-chrome");

  // ── Row 1: [Tabs (left)] ──────────── [Window Controls (right)] ────────
  tab_row_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_name(tab_row_, "tab-row");

  // Tab strip takes available space on the left
  GtkWidget *tab_widget = tab_strip_->get_widget();
  gtk_widget_set_hexpand(tab_widget, TRUE);
  gtk_box_append(GTK_BOX(tab_row_), tab_widget);

  // Window controls pinned to the right
  GtkWidget *wctl_widget = window_controls_->get_widget();
  gtk_widget_set_halign(wctl_widget, GTK_ALIGN_END);
  gtk_widget_set_valign(wctl_widget, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(tab_row_), wctl_widget);

  gtk_box_append(GTK_BOX(main_vbox_), tab_row_);

  // ── Row 2: [Nav] [Address Bar] [Menu] ─────────────────────────────────
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_name(toolbar, "toolbar");
  gtk_widget_set_margin_start(toolbar, 12);
  gtk_widget_set_margin_end(toolbar, 12);
  gtk_widget_set_margin_top(toolbar, 6);
  gtk_widget_set_margin_bottom(toolbar, 8);

  // Navigation buttons (left)
  gtk_box_append(GTK_BOX(toolbar), nav_bar_->get_widget());

  // Address bar (center, expands)
  GtkWidget *addr = address_bar_->get_widget();
  gtk_widget_set_hexpand(addr, TRUE);
  gtk_widget_set_margin_start(addr, 6);
  gtk_widget_set_margin_end(addr, 6);
  gtk_box_append(GTK_BOX(toolbar), addr);

  // Menu button (right)
  menu_button_ = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button_), "ob-menu");
  gtk_widget_set_name(menu_button_, "toolbar-menu");
  gtk_widget_set_tooltip_text(menu_button_, "Menu");
  gtk_box_append(GTK_BOX(toolbar), menu_button_);

  gtk_box_append(GTK_BOX(main_vbox_), toolbar);

  // ── Row 3: Content stack ───────────────────────────────────────────────
  content_stack_ = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(content_stack_),
                                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration(GTK_STACK(content_stack_), 80);
  gtk_widget_set_hexpand(content_stack_, TRUE);
  gtk_widget_set_vexpand(content_stack_, TRUE);
  gtk_box_append(GTK_BOX(main_vbox_), content_stack_);
}

} // namespace open_browser::ui
