#include "navigation_bar.h"

#include <glib.h>

namespace open_browser::ui {

NavigationBar::NavigationBar() {
  build_ui();
}

NavigationBar::~NavigationBar() {
  // GTK widgets are destroyed when their parent container is destroyed
}

void NavigationBar::build_ui() {
  container_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  back_button_ = gtk_button_new_from_icon_name("ob-arrow-left");
  gtk_widget_set_tooltip_text(back_button_, "Back (Alt+Left)");
  gtk_widget_set_sensitive(back_button_, FALSE);
  g_signal_connect(back_button_, "clicked", G_CALLBACK(on_back_clicked), this);
  gtk_box_append(GTK_BOX(container_), back_button_);

  forward_button_ = gtk_button_new_from_icon_name("ob-arrow-right");
  gtk_widget_set_tooltip_text(forward_button_, "Forward (Alt+Right)");
  gtk_widget_set_sensitive(forward_button_, FALSE);
  g_signal_connect(forward_button_, "clicked", G_CALLBACK(on_forward_clicked),
                   this);
  gtk_box_append(GTK_BOX(container_), forward_button_);

  reload_button_ = gtk_button_new_from_icon_name("ob-refresh-cw");
  gtk_widget_set_tooltip_text(reload_button_, "Reload (Ctrl+R)");
  g_signal_connect(reload_button_, "clicked", G_CALLBACK(on_reload_clicked),
                   this);
  gtk_box_append(GTK_BOX(container_), reload_button_);
}

void NavigationBar::set_can_go_back(bool can) {
  gtk_widget_set_sensitive(back_button_, can ? TRUE : FALSE);
}

void NavigationBar::set_can_go_forward(bool can) {
  gtk_widget_set_sensitive(forward_button_, can ? TRUE : FALSE);
}

void NavigationBar::set_loading(bool loading) {
  if (loading) {
    gtk_button_set_icon_name(GTK_BUTTON(reload_button_), "ob-x");
    gtk_widget_set_tooltip_text(reload_button_, "Stop loading");
  } else {
    gtk_button_set_icon_name(GTK_BUTTON(reload_button_), "ob-refresh-cw");
    gtk_widget_set_tooltip_text(reload_button_, "Reload (Ctrl+R)");
  }
}

// Static callbacks
void NavigationBar::on_back_clicked(GtkButton * /*button*/,
                                    gpointer user_data) {
  auto *self = static_cast<NavigationBar *>(user_data);
  if (self->on_back_)
    self->on_back_();
}

void NavigationBar::on_forward_clicked(GtkButton * /*button*/,
                                       gpointer user_data) {
  auto *self = static_cast<NavigationBar *>(user_data);
  if (self->on_forward_)
    self->on_forward_();
}

void NavigationBar::on_reload_clicked(GtkButton * /*button*/,
                                      gpointer user_data) {
  auto *self = static_cast<NavigationBar *>(user_data);
  if (self->on_reload_or_stop_)
    self->on_reload_or_stop_();
}

} // namespace open_browser::ui
