#include "address_bar.h"

#include <glib.h>

namespace open_browser::ui {

AddressBar::AddressBar() {
  build_ui();
}

AddressBar::~AddressBar() {
  // GTK widgets are destroyed when their parent container is destroyed
}

void AddressBar::build_ui() {
  container_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_hexpand(container_, TRUE);

  // Security icon (lock/unlock)
  security_icon_ = gtk_image_new_from_icon_name("ob-lock");
  gtk_image_set_pixel_size(GTK_IMAGE(security_icon_), 14);
  gtk_widget_set_opacity(security_icon_, 0.45);
  gtk_box_append(GTK_BOX(container_), security_icon_);

  // URL entry
  entry_ = gtk_entry_new();
  gtk_widget_set_name(entry_, "address-bar");
  gtk_widget_set_hexpand(entry_, TRUE);
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry_), "Search or enter address");
  gtk_entry_set_input_purpose(GTK_ENTRY(entry_), GTK_INPUT_PURPOSE_URL);
  g_signal_connect(entry_, "activate", G_CALLBACK(on_activate), this);
  gtk_box_append(GTK_BOX(container_), entry_);
}

void AddressBar::set_url(const std::string &url) {
  gtk_editable_set_text(GTK_EDITABLE(entry_), url.c_str());
}

void AddressBar::set_secure(bool secure) {
  gtk_image_set_from_icon_name(GTK_IMAGE(security_icon_),
                               secure ? "ob-lock" : "ob-lock-open");
  gtk_widget_set_opacity(security_icon_, secure ? 0.6 : 1.0);
}

void AddressBar::focus_and_select_all() {
  gtk_widget_grab_focus(entry_);
  gtk_editable_select_region(GTK_EDITABLE(entry_), 0, -1);
}

// Static callback
void AddressBar::on_activate(GtkEntry *entry, gpointer user_data) {
  auto *self = static_cast<AddressBar *>(user_data);
  const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
  if (text && *text && self->on_navigate_) {
    self->on_navigate_(std::string(text));
  }
}

} // namespace open_browser::ui
