#pragma once

#include <gtk/gtk.h>

#include <functional>

namespace open_browser::ui {

// Window control buttons (minimize, maximize, close)
// Positioned on the RIGHT side of the window
class WindowControls {
public:
  using Callback = std::function<void()>;

  WindowControls();
  ~WindowControls();

  // Get the widget container
  GtkWidget *get_widget() const { return container_; }

  // Set callbacks for window control actions
  void set_minimize_callback(Callback callback) { on_minimize_ = callback; }
  void set_maximize_callback(Callback callback) { on_maximize_ = callback; }
  void set_close_callback(Callback callback) { on_close_ = callback; }

  // Apply theme
  void apply_theme(bool dark_mode = false);

private:
  GtkWidget *container_ = nullptr;
  GtkWidget *minimize_button_ = nullptr;
  GtkWidget *maximize_button_ = nullptr;
  GtkWidget *close_button_ = nullptr;

  Callback on_minimize_;
  Callback on_maximize_;
  Callback on_close_;

  void build_ui();
  void setup_callbacks();

  // Static callbacks for GTK signals
  static void on_minimize_clicked(GtkButton *button, gpointer user_data);
  static void on_maximize_clicked(GtkButton *button, gpointer user_data);
  static void on_close_clicked(GtkButton *button, gpointer user_data);
};

} // namespace open_browser::ui
