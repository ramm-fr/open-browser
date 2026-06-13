#include "tab_strip.h"

#include <glib.h>

#include <string>

namespace open_browser::ui {

TabStrip::TabStrip() {
  build_ui();
}

TabStrip::~TabStrip() {
  // GTK widgets are destroyed when their parent container is destroyed
}

void TabStrip::build_ui() {
  // Outer box: [scrolled-tabs] [add-tab-button]
  container_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(container_, TRUE);
  gtk_widget_set_name(container_, "tab-bar-outer");

  scroll_ = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
  gtk_widget_set_hexpand(scroll_, TRUE);

  tabs_box_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_name(tabs_box_, "tab-bar");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll_), tabs_box_);

  gtk_box_append(GTK_BOX(container_), scroll_);

  // ── Add-tab button (+ at right of tab bar) ────────────────────────────
  new_tab_btn_ = gtk_button_new_from_icon_name("ob-plus");
  gtk_widget_set_name(new_tab_btn_, "tab-add-button");
  gtk_widget_set_tooltip_text(new_tab_btn_, "New Tab (Ctrl+T)");
  gtk_widget_set_valign(new_tab_btn_, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(new_tab_btn_, 4);
  gtk_widget_set_margin_end(new_tab_btn_, 6);
  g_signal_connect(new_tab_btn_, "clicked",
                   G_CALLBACK(on_new_tab_button_clicked), this);
  gtk_box_append(GTK_BOX(container_), new_tab_btn_);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab management
// ─────────────────────────────────────────────────────────────────────────────

void TabStrip::clear() {
  GtkWidget *child = gtk_widget_get_first_child(tabs_box_);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    gtk_box_remove(GTK_BOX(tabs_box_), child);
    child = next;
  }
}

void TabStrip::add_tab(int tab_id, const std::string &title, bool loading,
                       bool active) {
  // ── Outer wrapper: [tab-btn] [close-btn] side-by-side ────────────────
  GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(outer, "tab-outer");
  if (active) {
    gtk_widget_add_css_class(outer, "active-tab-outer");
  }

  // ── Tab switch button (icon + title) ──────────────────────────────────
  GtkWidget *inner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // Favicon / spinner
  if (loading) {
    GtkWidget *spinner = gtk_spinner_new();
    gtk_spinner_start(GTK_SPINNER(spinner));
    gtk_widget_set_size_request(spinner, 14, 14);
    gtk_box_append(GTK_BOX(inner), spinner);
  } else {
    GtkWidget *favicon = gtk_image_new_from_icon_name("ob-globe");
    gtk_image_set_pixel_size(GTK_IMAGE(favicon), 14);
    gtk_widget_set_opacity(favicon, 0.5);
    gtk_box_append(GTK_BOX(inner), favicon);
  }

  // Title label
  std::string label_text = title.empty() ? "New Tab" : title;
  if (label_text.size() > 20)
    label_text = label_text.substr(0, 18) + "\xe2\x80\xa6"; // "…"
  GtkWidget *label = gtk_label_new(label_text.c_str());
  gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
  gtk_box_append(GTK_BOX(inner), label);

  GtkWidget *tab_btn = gtk_button_new();
  gtk_button_set_child(GTK_BUTTON(tab_btn), inner);
  gtk_widget_add_css_class(tab_btn, "tab-button");
  if (active) {
    gtk_widget_add_css_class(tab_btn, "active-tab");
  }

  // Switch signal
  g_object_set_data(G_OBJECT(tab_btn), "tab-id", GINT_TO_POINTER(tab_id));
  g_signal_connect(
      tab_btn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer ud) {
        auto *self = static_cast<TabStrip *>(ud);
        int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "tab-id"));
        if (self->on_tab_clicked_)
          self->on_tab_clicked_(id);
      }),
      this);

  // ── Close button — separate widget, NOT inside tab_btn ────────────────
  GtkWidget *close_btn = gtk_button_new_from_icon_name("ob-x");
  gtk_widget_add_css_class(close_btn, "tab-close");
  gtk_widget_set_tooltip_text(close_btn, "Close tab (Ctrl+W)");
  gtk_widget_set_valign(close_btn, GTK_ALIGN_CENTER);

  g_object_set_data(G_OBJECT(close_btn), "tab-id", GINT_TO_POINTER(tab_id));
  g_signal_connect(
      close_btn, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer ud) {
        auto *self = static_cast<TabStrip *>(ud);
        int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "tab-id"));
        if (self->on_tab_closed_)
          self->on_tab_closed_(id);
      }),
      this);

  gtk_box_append(GTK_BOX(outer), tab_btn);
  gtk_box_append(GTK_BOX(outer), close_btn);
  gtk_box_append(GTK_BOX(tabs_box_), outer);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static callbacks
// ─────────────────────────────────────────────────────────────────────────────

void TabStrip::on_new_tab_button_clicked(GtkButton * /*button*/,
                                         gpointer user_data) {
  auto *self = static_cast<TabStrip *>(user_data);
  if (self->on_new_tab_)
    self->on_new_tab_();
}

} // namespace open_browser::ui
