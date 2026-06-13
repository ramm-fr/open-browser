#include "window_controls.h"

#include <glib.h>

namespace open_browser::ui {

WindowControls::WindowControls() {
  build_ui();
  setup_callbacks();
}

WindowControls::~WindowControls() {
  // GTK widgets will be destroyed when their parent container is destroyed
}

void WindowControls::build_ui() {
  // Create horizontal box container for window controls
  container_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  // Set container properties
  gtk_widget_set_name(container_, "window-controls");
  gtk_widget_set_halign(container_, GTK_ALIGN_END);
  gtk_widget_set_valign(container_, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start(container_, 4);

  // Create minimize button
  minimize_button_ = gtk_button_new_from_icon_name("ob-circle-minus");
  gtk_widget_add_css_class(minimize_button_, "wctl-btn");
  gtk_widget_add_css_class(minimize_button_, "wctl-min");
  gtk_widget_set_tooltip_text(minimize_button_, "Minimize");

  // Create maximize button
  maximize_button_ = gtk_button_new_from_icon_name("ob-maximize");
  gtk_widget_add_css_class(maximize_button_, "wctl-btn");
  gtk_widget_add_css_class(maximize_button_, "wctl-max");
  gtk_widget_set_tooltip_text(maximize_button_, "Maximize");

  // Create close button
  close_button_ = gtk_button_new_from_icon_name("ob-circle-x");
  gtk_widget_add_css_class(close_button_, "wctl-btn");
  gtk_widget_add_css_class(close_button_, "wctl-close");
  gtk_widget_set_tooltip_text(close_button_, "Close");

  // Add buttons to container (in order: minimize, maximize, close)
  gtk_box_append(GTK_BOX(container_), minimize_button_);
  gtk_box_append(GTK_BOX(container_), maximize_button_);
  gtk_box_append(GTK_BOX(container_), close_button_);
}

void WindowControls::setup_callbacks() {
  g_signal_connect(minimize_button_, "clicked", G_CALLBACK(on_minimize_clicked),
                   this);
  g_signal_connect(maximize_button_, "clicked", G_CALLBACK(on_maximize_clicked),
                   this);
  g_signal_connect(close_button_, "clicked", G_CALLBACK(on_close_clicked),
                   this);
}

void WindowControls::apply_theme(bool dark_mode) {
  // Theme is applied via CSS, which is loaded separately
  // This method can be used to trigger theme updates if needed
  (void)dark_mode;
}

// Static callback implementations
void WindowControls::on_minimize_clicked(GtkButton *button,
                                         gpointer user_data) {
  (void)button;
  WindowControls *self = static_cast<WindowControls *>(user_data);
  if (self->on_minimize_) {
    self->on_minimize_();
  }
}

void WindowControls::on_maximize_clicked(GtkButton *button,
                                         gpointer user_data) {
  (void)button;
  WindowControls *self = static_cast<WindowControls *>(user_data);
  if (self->on_maximize_) {
    self->on_maximize_();
  }
}

void WindowControls::on_close_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  WindowControls *self = static_cast<WindowControls *>(user_data);
  if (self->on_close_) {
    self->on_close_();
  }
}

} // namespace open_browser::ui
