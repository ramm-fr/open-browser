#pragma once

#include <gtk/gtk.h>

#include <functional>

namespace open_browser::ui {

// Reusable navigation bar component: [Back] [Forward] [Reload/Stop]
// Produces an HBox with 2px spacing matching the existing layout.
class NavigationBar {
public:
  using Callback = std::function<void()>;

  NavigationBar();
  ~NavigationBar();

  // Get the root widget (HBox) for packing into the header bar
  GtkWidget *get_widget() const { return container_; }

  // Set callbacks
  void set_back_callback(Callback callback) { on_back_ = std::move(callback); }
  void set_forward_callback(Callback callback) {
    on_forward_ = std::move(callback);
  }
  void set_reload_or_stop_callback(Callback callback) {
    on_reload_or_stop_ = std::move(callback);
  }

  // Update button sensitivity
  void set_can_go_back(bool can);
  void set_can_go_forward(bool can);

  // Toggle reload ↔ stop icon and tooltip
  void set_loading(bool loading);

private:
  GtkWidget *container_ = nullptr;
  GtkWidget *back_button_ = nullptr;
  GtkWidget *forward_button_ = nullptr;
  GtkWidget *reload_button_ = nullptr;

  Callback on_back_;
  Callback on_forward_;
  Callback on_reload_or_stop_;

  void build_ui();

  // Static signal callbacks
  static void on_back_clicked(GtkButton *button, gpointer user_data);
  static void on_forward_clicked(GtkButton *button, gpointer user_data);
  static void on_reload_clicked(GtkButton *button, gpointer user_data);
};

} // namespace open_browser::ui
