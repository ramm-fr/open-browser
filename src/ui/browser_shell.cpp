#include "browser_shell.h"

#include <glib.h>

namespace open_browser::ui {

BrowserShell::BrowserShell(GtkWindow *window) {
  build_ui(window);
}

BrowserShell::~BrowserShell() {
  // GTK widgets are destroyed when their parent window is destroyed.
  // unique_ptr members are cleaned up automatically.
}

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────

void BrowserShell::set_callbacks(const Callbacks &cb) {
  // Navigation
  nav_bar_->set_back_callback(cb.on_back);
  nav_bar_->set_forward_callback(cb.on_forward);
  nav_bar_->set_reload_or_stop_callback(cb.on_reload_or_stop);

  // Address bar
  address_bar_->set_navigate_callback(cb.on_navigate);

  // Tabs
  tab_strip_->set_tab_clicked_callback(cb.on_tab_clicked);
  tab_strip_->set_tab_closed_callback(cb.on_tab_closed);
  tab_strip_->set_new_tab_callback(cb.on_new_tab);

  // Window controls
  window_controls_->set_minimize_callback(cb.on_minimize);
  window_controls_->set_maximize_callback(cb.on_maximize);
  window_controls_->set_close_callback(cb.on_close);
}

// ─────────────────────────────────────────────────────────────────────────────
// Address bar delegations
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

// ─────────────────────────────────────────────────────────────────────────────
// Navigation bar delegations
// ─────────────────────────────────────────────────────────────────────────────

void BrowserShell::set_can_go_back(bool can) {
  nav_bar_->set_can_go_back(can);
}

void BrowserShell::set_can_go_forward(bool can) {
  nav_bar_->set_can_go_forward(can);
}

void BrowserShell::set_loading(bool loading) {
  nav_bar_->set_loading(loading);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab strip delegations
// ─────────────────────────────────────────────────────────────────────────────

void BrowserShell::clear_tabs() {
  tab_strip_->clear();
}

void BrowserShell::add_tab(int tab_id, const std::string &title, bool loading,
                           bool active) {
  tab_strip_->add_tab(tab_id, title, loading, active);
}

// ─────────────────────────────────────────────────────────────────────────────
// UI Construction — Safari-inspired layout
//
// Row 1: [GtkHeaderBar — minimal, just window controls on right]
// Row 2: [TabStrip — rounded tabs + new tab button]
// Row 3: [Toolbar — back/forward/reload | address bar | menu]
// Row 4: [GtkStack — web content]
// ─────────────────────────────────────────────────────────────────────────────

void BrowserShell::build_ui(GtkWindow *window) {
  // ── Create components ──────────────────────────────────────────────────
  nav_bar_ = std::make_unique<NavigationBar>();
  address_bar_ = std::make_unique<AddressBar>();
  tab_strip_ = std::make_unique<TabStrip>();
  window_controls_ = std::make_unique<WindowControls>();

  // ── Row 1: Header bar (title bar) — only window controls ───────────────
  header_bar_ = gtk_header_bar_new();
  gtk_widget_set_name(header_bar_, "shell-titlebar");
  gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header_bar_), FALSE);
  gtk_window_set_titlebar(window, header_bar_);

  // Empty title widget (no address bar here in new layout)
  GtkWidget *title_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(title_spacer, TRUE);
  gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header_bar_), title_spacer);

  // Window controls on the right
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar_),
                          window_controls_->get_widget());

  // ── Main vertical box ──────────────────────────────────────────────────
  main_vbox_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_name(main_vbox_, "shell-vbox");

  // ── Row 2: Tab strip ───────────────────────────────────────────────────
  gtk_box_append(GTK_BOX(main_vbox_), tab_strip_->get_widget());

  // ── Row 3: Toolbar — [nav buttons] [address bar] [menu] ───────────────
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_name(toolbar, "shell-toolbar");
  gtk_widget_set_margin_start(toolbar, 14);
  gtk_widget_set_margin_end(toolbar, 14);
  gtk_widget_set_margin_top(toolbar, 7);
  gtk_widget_set_margin_bottom(toolbar, 9);

  // Navigation buttons
  gtk_box_append(GTK_BOX(toolbar), nav_bar_->get_widget());

  // Address bar (expands to fill)
  GtkWidget *addr_widget = address_bar_->get_widget();
  gtk_widget_set_hexpand(addr_widget, TRUE);
  gtk_box_append(GTK_BOX(toolbar), addr_widget);

  // Menu button
  menu_button_ = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button_), "ob-menu");
  gtk_widget_set_name(menu_button_, "shell-menu-button");
  gtk_widget_set_tooltip_text(menu_button_, "Menu");
  gtk_box_append(GTK_BOX(toolbar), menu_button_);

  gtk_box_append(GTK_BOX(main_vbox_), toolbar);

  // ── Row 4: Content stack ───────────────────────────────────────────────
  content_stack_ = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(content_stack_),
                                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration(GTK_STACK(content_stack_), 100);
  gtk_widget_set_hexpand(content_stack_, TRUE);
  gtk_widget_set_vexpand(content_stack_, TRUE);
  gtk_box_append(GTK_BOX(main_vbox_), content_stack_);
}

} // namespace open_browser::ui
