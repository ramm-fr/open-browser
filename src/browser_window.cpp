// Open Browser — browser_window.cpp
// Complete GTK4 + WebKitGTK4 browser window implementation.

#include "browser_window.h"

#include "ad_blocker.h"
#include "history_manager.h"
#include "settings_manager.h"
#include "url_resolver.h"

#include <glib.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace open_browser {

namespace {

// Path to the internal pages directory (relative to binary, or installed).
std::string find_pages_dir() {
  // Try installed location first
  const char *data_dirs =
      g_get_system_data_dirs() ? g_get_system_data_dirs()[0] : nullptr;
  if (data_dirs) {
    std::filesystem::path installed =
        std::filesystem::path(data_dirs) / "open-browser" / "pages";
    if (std::filesystem::exists(installed)) {
      return installed.string();
    }
  }
  // Development: look next to the binary
  const std::filesystem::path exe_dir =
      std::filesystem::canonical("/proc/self/exe").parent_path();
  std::filesystem::path dev_path = exe_dir / "pages";
  if (std::filesystem::exists(dev_path)) {
    return dev_path.string();
  }
  // Source tree fallback
  dev_path = exe_dir / ".." / "src" / "ui" / "pages";
  if (std::filesystem::exists(dev_path)) {
    return std::filesystem::canonical(dev_path).string();
  }
  return "";
}

// Convert an internal openbrowser:// URL to a file:// URI.
std::string internal_to_file_uri(const std::string &url) {
  static const std::string pages_dir = find_pages_dir();

  auto strip_scheme = [](const std::string &u) -> std::string {
    const std::string prefix = "openbrowser://";
    if (u.starts_with(prefix))
      return u.substr(prefix.size());
    return u;
  };

  const std::string page = strip_scheme(url);

  // Map page names to HTML files
  const std::unordered_map<std::string, std::string> page_map = {
      {"newtab", "newtab/index.html"},
      {"settings", "settings/index.html"},
      {"bookmarks", "bookmarks/index.html"},
      {"history", "history/index.html"},
      {"downloads", "downloads/index.html"},
      {"about", "about/index.html"},
  };

  auto it = page_map.find(page);
  if (it != page_map.end() && !pages_dir.empty()) {
    const std::string full_path = pages_dir + "/" + it->second;
    return "file://" + full_path;
  }

  // Fallback: render a minimal error page inline
  return "data:text/html,<html><body><h1>Page not found</h1></body></html>";
}

// Path to the shell CSS theme file (looked up at runtime).
std::string find_theme_file(const std::string &filename) {
  const std::filesystem::path exe_dir =
      std::filesystem::canonical("/proc/self/exe").parent_path();

  // Development: next to the binary or in the source tree
  for (const auto &candidate : {
           exe_dir / ".." / "src" / "themes" / filename,
           exe_dir / "themes" / filename,
       }) {
    if (std::filesystem::exists(candidate))
      return std::filesystem::canonical(candidate).string();
  }

  // Installed location
  const char *data_dirs =
      g_get_system_data_dirs() ? g_get_system_data_dirs()[0] : nullptr;
  if (data_dirs) {
    std::filesystem::path installed =
        std::filesystem::path(data_dirs) / "open-browser" / "themes" / filename;
    if (std::filesystem::exists(installed))
      return installed.string();
  }
  return "";
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

BrowserWindow::BrowserWindow(GtkApplication *app, bool private_mode)
    : app_(app), private_mode_(private_mode) {
  // Load ad block lists in background
  if (SettingsManager::instance().block_ads() ||
      SettingsManager::instance().block_trackers()) {
    AdBlocker::instance().load_filter_lists();
  }

  build_ui();
}

BrowserWindow::~BrowserWindow() {
  if (css_provider_) {
    gtk_style_context_remove_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider_));
    g_object_unref(css_provider_);
  }
  if (webkit_settings_)
    g_object_unref(webkit_settings_);
  if (user_content_manager_)
    g_object_unref(user_content_manager_);
  if (dark_css_provider_)
    g_object_unref(dark_css_provider_);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void BrowserWindow::show() {
  gtk_window_present(GTK_WINDOW(window_));
}

void BrowserWindow::navigate(const std::string &url) {
  Tab *tab = active_tab();
  if (!tab)
    return;

  const std::string resolved =
      UrlResolver::resolve(url, SettingsManager::instance().search_engine());
  tab->url = resolved;

  if (UrlResolver::is_internal_url(url)) {
    const std::string file_uri = internal_to_file_uri(url);
    webkit_web_view_load_uri(tab->webview, file_uri.c_str());
  } else {
    webkit_web_view_load_uri(tab->webview, resolved.c_str());
  }
  update_address_bar(url);
}

void BrowserWindow::new_tab(const std::string &url) {
  Tab tab;
  tab.id = next_tab_id_++;
  tab.url = url;
  tab.title = "New Tab";
  tab.loading = false;
  tab.private_mode = private_mode_;

  tab.page_widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(tab.page_widget, TRUE);
  gtk_widget_set_vexpand(tab.page_widget, TRUE);

  setup_webview_for_tab(tab);

  const std::string page_name = "tab-" + std::to_string(tab.id);
  gtk_stack_add_named(GTK_STACK(webview_stack_), tab.page_widget,
                      page_name.c_str());

  const int new_id = tab.id;
  tabs_.push_back(std::move(tab));

  // Set active first, then navigate so active_tab() resolves correctly
  active_tab_id_ = new_id;
  gtk_stack_set_visible_child_name(GTK_STACK(webview_stack_),
                                   page_name.c_str());
  navigate(url);
  rebuild_tab_bar_buttons();
}

void BrowserWindow::close_tab(int tab_id) {
  if (tabs_.size() == 1) {
    // Last tab — replace it with a fresh new tab instead of closing the window
    navigate("openbrowser://newtab");
    return;
  }

  auto it = std::find_if(tabs_.begin(), tabs_.end(),
                         [tab_id](const Tab &t) { return t.id == tab_id; });
  if (it == tabs_.end())
    return;

  const std::string page_name = "tab-" + std::to_string(tab_id);
  GtkWidget *page =
      gtk_stack_get_child_by_name(GTK_STACK(webview_stack_), page_name.c_str());
  if (page) {
    gtk_stack_remove(GTK_STACK(webview_stack_), page);
  }

  // If we're closing the active tab, switch to an adjacent one first
  if (tab_id == active_tab_id_) {
    int new_active =
        (it == tabs_.begin()) ? std::next(it)->id : std::prev(it)->id;
    active_tab_id_ = 0;
    tabs_.erase(it);
    switch_tab(new_active);
  } else {
    tabs_.erase(it);
  }

  rebuild_tab_bar_buttons();
}

void BrowserWindow::switch_tab(int tab_id) {
  Tab *tab = find_tab(tab_id);
  if (!tab)
    return;

  active_tab_id_ = tab_id;

  const std::string page_name = "tab-" + std::to_string(tab_id);
  gtk_stack_set_visible_child_name(GTK_STACK(webview_stack_),
                                   page_name.c_str());

  update_address_bar(tab->url);
  update_title(tab->title);
  update_navigation_buttons();
  rebuild_tab_bar_buttons();
}

void BrowserWindow::go_back() {
  Tab *tab = active_tab();
  if (tab && webkit_web_view_can_go_back(tab->webview)) {
    webkit_web_view_go_back(tab->webview);
  }
}

void BrowserWindow::go_forward() {
  Tab *tab = active_tab();
  if (tab && webkit_web_view_can_go_forward(tab->webview)) {
    webkit_web_view_go_forward(tab->webview);
  }
}

void BrowserWindow::reload() {
  Tab *tab = active_tab();
  if (tab)
    webkit_web_view_reload(tab->webview);
}

void BrowserWindow::stop() {
  Tab *tab = active_tab();
  if (tab)
    webkit_web_view_stop_loading(tab->webview);
}

void BrowserWindow::focus_address_bar() {
  shell_->focus_address_bar();
}

void BrowserWindow::toggle_private_mode() {
  private_mode_ = !private_mode_;
  if (private_mode_) {
    gtk_widget_add_css_class(window_, "private-mode");
  } else {
    gtk_widget_remove_css_class(window_, "private-mode");
  }
}

void BrowserWindow::open_downloads() {
  new_tab("openbrowser://downloads");
}

void BrowserWindow::open_settings() {
  new_tab("openbrowser://settings");
}

void BrowserWindow::open_bookmarks() {
  new_tab("openbrowser://bookmarks");
}

void BrowserWindow::open_history() {
  new_tab("openbrowser://history");
}

void BrowserWindow::open_about() {
  new_tab("openbrowser://about");
}

// ─────────────────────────────────────────────────────────────────────────────
// UI Construction
// ─────────────────────────────────────────────────────────────────────────────

void BrowserWindow::build_ui() {
  apply_css();

  // Root window
  window_ = gtk_application_window_new(app_);
  gtk_window_set_title(GTK_WINDOW(window_), "Open Browser");
  gtk_window_set_default_size(GTK_WINDOW(window_), 1280, 800);
  // Set WM class so taskbar shows correct icon matching .desktop file
  gtk_window_set_application(GTK_WINDOW(window_), GTK_APPLICATION(app_));
  // Load app icon from our bundled SVG directly — avoids system icon cache
  // issues
  {
    const std::filesystem::path exe_dir =
        std::filesystem::canonical("/proc/self/exe").parent_path();
    const std::vector<std::string> icon_paths = {
        (exe_dir / ".." / "resources" / "icons" / "hicolor" / "scalable" /
         "apps" / "io.openbrowser.Browser.svg")
            .string(),
        "/usr/share/open-browser/icons/hicolor/scalable/apps/"
        "io.openbrowser.Browser.svg",
        "/usr/share/icons/hicolor/scalable/apps/io.openbrowser.Browser.svg",
    };
    for (const auto &path : icon_paths) {
      if (std::filesystem::exists(path)) {
        gtk_window_set_icon_name(GTK_WINDOW(window_), "io.openbrowser.Browser");
        break;
      }
    }
  }
  gtk_widget_add_css_class(window_, "open-browser-window");
  if (private_mode_) {
    gtk_widget_add_css_class(window_, "private-mode");
  }

  // Store self pointer for external access
  g_object_set_data(G_OBJECT(window_), "open-browser-window", this);

  // Keyboard controller
  GtkEventController *key_ctrl = gtk_event_controller_key_new();
  g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), this);
  gtk_widget_add_controller(window_, key_ctrl);

  // ── Build the browser shell (header bar + tab strip + content stack) ───
  shell_ = std::make_unique<ui::BrowserShell>(GTK_WINDOW(window_));

  shell_->set_callbacks({
      .on_back = [this]() { go_back(); },
      .on_forward = [this]() { go_forward(); },
      .on_reload_or_stop =
          [this]() {
            Tab *tab = active_tab();
            if (tab && tab->loading) {
              stop();
            } else {
              reload();
            }
          },
      .on_navigate = [this](const std::string &url) { navigate(url); },
      .on_tab_clicked = [this](int tab_id) { switch_tab(tab_id); },
      .on_tab_closed = [this](int tab_id) { close_tab(tab_id); },
      .on_new_tab = [this]() { new_tab(); },
      .on_minimize = [this]() { gtk_window_minimize(GTK_WINDOW(window_)); },
      .on_maximize =
          [this]() {
            if (gtk_window_is_maximized(GTK_WINDOW(window_))) {
              gtk_window_unmaximize(GTK_WINDOW(window_));
            } else {
              gtk_window_maximize(GTK_WINDOW(window_));
            }
          },
      .on_close = [this]() { gtk_window_destroy(GTK_WINDOW(window_)); },
  });

  // Set root vbox as window child
  gtk_window_set_child(GTK_WINDOW(window_), shell_->get_root_vbox());

  // Keep a convenience pointer to the content stack
  webview_stack_ = shell_->get_content_stack();

  // Set up menu and window actions
  setup_menu_and_actions();

  // Open the first tab
  new_tab("openbrowser://newtab");
}

void BrowserWindow::setup_menu_and_actions() {
  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Settings", "win.settings");
  g_menu_append(menu, "Bookmarks", "win.bookmarks");
  g_menu_append(menu, "History", "win.history");
  g_menu_append(menu, "Downloads", "win.downloads");
  g_menu_append(menu, "New Private Window", "win.new-private");

  GMenu *zoom_section = g_menu_new();
  g_menu_append(zoom_section, "Zoom In", "win.zoom-in");
  g_menu_append(zoom_section, "Zoom Out", "win.zoom-out");
  g_menu_append(zoom_section, "Actual Size", "win.zoom-reset");
  g_menu_append_section(menu, "Zoom", G_MENU_MODEL(zoom_section));
  g_object_unref(zoom_section);

  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(shell_->get_menu_button()),
                                 G_MENU_MODEL(menu));
  g_object_unref(menu);

  // Register window actions
  auto add_action = [this](const char *name, GCallback cb) {
    GSimpleAction *action = g_simple_action_new(name, nullptr);
    g_signal_connect(action, "activate", cb, this);
    g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(action));
    g_object_unref(action);
  };

  add_action("settings", G_CALLBACK(on_menu_settings));
  add_action("bookmarks", G_CALLBACK(on_menu_bookmarks));
  add_action("history", G_CALLBACK(on_menu_history));
  add_action("downloads", G_CALLBACK(on_menu_downloads));
  add_action("new-private", G_CALLBACK(on_menu_new_private));
  add_action("zoom-in", G_CALLBACK(on_menu_zoom_in));
  add_action("zoom-out", G_CALLBACK(on_menu_zoom_out));
  add_action("zoom-reset", G_CALLBACK(on_menu_zoom_reset));
}

void BrowserWindow::setup_webview_for_tab(Tab &tab) {
  // Create shared WebKit settings once
  if (!webkit_settings_) {
    webkit_settings_ = webkit_settings_new();
    webkit_settings_set_enable_javascript(webkit_settings_, TRUE);
    webkit_settings_set_enable_smooth_scrolling(webkit_settings_, TRUE);
    webkit_settings_set_enable_webgl(webkit_settings_, TRUE);
    webkit_settings_set_enable_media(webkit_settings_, TRUE);
    webkit_settings_set_javascript_can_open_windows_automatically(
        webkit_settings_, FALSE);
    webkit_settings_set_enable_developer_extras(webkit_settings_, TRUE);
    webkit_settings_set_allow_modal_dialogs(webkit_settings_, FALSE);
    webkit_settings_set_media_playback_requires_user_gesture(webkit_settings_,
                                                             TRUE);

    const bool hw = SettingsManager::instance().hardware_acceleration();
    webkit_settings_set_hardware_acceleration_policy(
        webkit_settings_, hw ? WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
                             : WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

    const int font_size = SettingsManager::instance().font_size();
    webkit_settings_set_default_font_size(webkit_settings_,
                                          static_cast<guint32>(font_size));
  }

  // Create network session (once per window; private mode uses ephemeral
  // session)
  WebKitNetworkSession *network_session = nullptr;
  if (tab.private_mode) {
    network_session = webkit_network_session_new_ephemeral();
  } else {
    network_session = webkit_network_session_new(nullptr, nullptr);
  }

  // Create a shared UserContentManager once — registers script message handler
  if (!user_content_manager_) {
    user_content_manager_ = webkit_user_content_manager_new();
    webkit_user_content_manager_register_script_message_handler(
        user_content_manager_, "obBridge", nullptr);
    g_signal_connect(user_content_manager_, "script-message-received::obBridge",
                     G_CALLBACK(on_script_message), this);
  }

  // Create the WebView
  tab.webview = WEBKIT_WEB_VIEW(g_object_new(
      WEBKIT_TYPE_WEB_VIEW, "settings", webkit_settings_, "network-session",
      network_session, "user-content-manager", user_content_manager_, nullptr));
  g_object_unref(network_session);

  gtk_widget_set_hexpand(GTK_WIDGET(tab.webview), TRUE);
  gtk_widget_set_vexpand(GTK_WIDGET(tab.webview), TRUE);

  // Connect signals
  g_signal_connect(tab.webview, "load-changed", G_CALLBACK(on_load_changed),
                   this);
  g_signal_connect(tab.webview, "notify::title", G_CALLBACK(on_title_changed),
                   this);
  g_signal_connect(tab.webview, "notify::uri", G_CALLBACK(on_uri_changed),
                   this);
  g_signal_connect(tab.webview, "decide-policy", G_CALLBACK(on_decide_policy),
                   this);
  g_signal_connect(tab.webview, "create", G_CALLBACK(on_create_webview), this);

  // Add webview to the page container
  gtk_box_append(GTK_BOX(tab.page_widget), GTK_WIDGET(tab.webview));
  gtk_widget_set_visible(tab.page_widget, TRUE);
}

void BrowserWindow::apply_css() {
  css_provider_ = gtk_css_provider_new();

  // Load theme from external CSS file
  const std::string css_path = find_theme_file("shell.css");
  if (!css_path.empty()) {
    GFile *file = g_file_new_for_path(css_path.c_str());
    gtk_css_provider_load_from_file(css_provider_, file);
    g_object_unref(file);
  }

  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider_),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  // Add our custom Lucide icon theme search path
  GtkIconTheme *theme =
      gtk_icon_theme_get_for_display(gdk_display_get_default());

  // Development path: icons/ next to the binary
  const std::filesystem::path exe_dir =
      std::filesystem::canonical("/proc/self/exe").parent_path();
  const std::string dev_icons =
      (exe_dir / ".." / "resources" / "icons").string();
  if (std::filesystem::exists(dev_icons)) {
    gtk_icon_theme_add_search_path(theme, dev_icons.c_str());
  }

  // Installed path
  const char *data_dirs =
      g_get_system_data_dirs() ? g_get_system_data_dirs()[0] : nullptr;
  if (data_dirs) {
    const std::string installed_icons =
        std::string(data_dirs) + "/open-browser/icons";
    gtk_icon_theme_add_search_path(theme, installed_icons.c_str());
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// State helpers
// ─────────────────────────────────────────────────────────────────────────────

Tab *BrowserWindow::active_tab() {
  return find_tab(active_tab_id_);
}

const Tab *BrowserWindow::active_tab() const {
  return const_cast<BrowserWindow *>(this)->find_tab(active_tab_id_);
}

Tab *BrowserWindow::find_tab(int id) {
  auto it = std::find_if(tabs_.begin(), tabs_.end(),
                         [id](const Tab &t) { return t.id == id; });
  return it != tabs_.end() ? &(*it) : nullptr;
}

Tab *BrowserWindow::find_tab_by_webview(WebKitWebView *wv) {
  auto it = std::find_if(tabs_.begin(), tabs_.end(),
                         [wv](const Tab &t) { return t.webview == wv; });
  return it != tabs_.end() ? &(*it) : nullptr;
}

void BrowserWindow::update_navigation_buttons() {
  const Tab *tab = active_tab();
  if (!tab) {
    shell_->set_can_go_back(false);
    shell_->set_can_go_forward(false);
    return;
  }
  shell_->set_can_go_back(webkit_web_view_can_go_back(tab->webview));
  shell_->set_can_go_forward(webkit_web_view_can_go_forward(tab->webview));
}

void BrowserWindow::update_address_bar(const std::string &url) {
  // Strip internal scheme for display
  const std::string display = url.starts_with("openbrowser://") ? url : url;
  shell_->set_url(display);

  // Security indicator
  const bool secure = url.starts_with("https://") ||
                      url.starts_with("openbrowser://") ||
                      url.starts_with("file://") || url.starts_with("data:");
  shell_->set_secure(secure);
}

void BrowserWindow::update_title(const std::string &title) {
  const std::string window_title =
      title.empty() ? "Open Browser" : title + " — Open Browser";
  gtk_window_set_title(GTK_WINDOW(window_), window_title.c_str());
}

void BrowserWindow::update_tab_label(int tab_id, const std::string &title,
                                     bool loading) {
  // We rebuild tab buttons as a simple approach for correctness.
  // A more optimised approach would update the label in place.
  (void)tab_id;
  (void)title;
  (void)loading;
  rebuild_tab_bar_buttons();
}

void BrowserWindow::rebuild_tab_bar_buttons() {
  shell_->clear_tabs();
  for (const Tab &tab : tabs_) {
    shell_->add_tab(tab.id, tab.title, tab.loading, tab.id == active_tab_id_);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Signal Handlers
// ─────────────────────────────────────────────────────────────────────────────

gboolean BrowserWindow::on_key_pressed(GtkEventControllerKey * /*controller*/,
                                       guint keyval, guint /*keycode*/,
                                       GdkModifierType state,
                                       gpointer user_data) {
  auto *self = static_cast<BrowserWindow *>(user_data);
  const bool ctrl = (state & GDK_CONTROL_MASK) != 0;
  const bool shift = (state & GDK_SHIFT_MASK) != 0;
  const bool alt = (state & GDK_ALT_MASK) != 0;

  if (ctrl && keyval == GDK_KEY_t) {
    self->new_tab();
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_w) {
    if (auto *t = self->active_tab())
      self->close_tab(t->id);
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_l) {
    self->focus_address_bar();
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_r) {
    self->reload();
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_d) { /* bookmark */
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_h) {
    self->open_history();
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_j) {
    self->open_downloads();
    return TRUE;
  }
  if (ctrl && keyval == GDK_KEY_comma) {
    self->open_settings();
    return TRUE;
  }
  if (alt && keyval == GDK_KEY_Left) {
    self->go_back();
    return TRUE;
  }
  if (alt && keyval == GDK_KEY_Right) {
    self->go_forward();
    return TRUE;
  }
  if (ctrl && shift && keyval == GDK_KEY_N) {
    auto *new_win = new BrowserWindow(self->app_, true);
    new_win->show();
    return TRUE;
  }
  // Ctrl+Tab / Ctrl+Shift+Tab
  if (ctrl && keyval == GDK_KEY_Tab) {
    if (!self->tabs_.empty()) {
      auto it = std::find_if(
          self->tabs_.begin(), self->tabs_.end(),
          [self](const Tab &t) { return t.id == self->active_tab_id_; });
      if (shift) {
        if (it == self->tabs_.begin())
          it = self->tabs_.end();
        --it;
      } else {
        ++it;
        if (it == self->tabs_.end())
          it = self->tabs_.begin();
      }
      self->switch_tab(it->id);
    }
    return TRUE;
  }
  return FALSE;
}

void BrowserWindow::on_load_changed(WebKitWebView *web_view,
                                    WebKitLoadEvent load_event,
                                    gpointer user_data) {
  auto *self = static_cast<BrowserWindow *>(user_data);
  Tab *tab = self->find_tab_by_webview(web_view);
  if (!tab)
    return;

  switch (load_event) {
  case WEBKIT_LOAD_STARTED:
    tab->loading = true;
    if (tab->id == self->active_tab_id_) {
      self->shell_->set_loading(true);
    }
    break;

  case WEBKIT_LOAD_COMMITTED: {
    const char *uri = webkit_web_view_get_uri(web_view);
    if (uri) {
      tab->url = uri;
      if (tab->id == self->active_tab_id_) {
        self->update_address_bar(tab->url);
        self->update_navigation_buttons();
      }
    }
    break;
  }

  case WEBKIT_LOAD_FINISHED:
    tab->loading = false;
    if (tab->id == self->active_tab_id_) {
      self->shell_->set_loading(false);
      self->update_navigation_buttons();
    }
    // Record history (skip internal pages)
    if (!tab->url.starts_with("openbrowser://") &&
        !tab->url.starts_with("file://") && !tab->url.starts_with("data:")) {
      HistoryManager::instance().add_visit(tab->url, tab->title);
    }
    self->update_tab_label(tab->id, tab->title, false);
    break;

  default:
    break;
  }
}

void BrowserWindow::on_title_changed(WebKitWebView *web_view,
                                     GParamSpec * /*pspec*/,
                                     gpointer user_data) {
  auto *self = static_cast<BrowserWindow *>(user_data);
  Tab *tab = self->find_tab_by_webview(web_view);
  if (!tab)
    return;

  const char *title = webkit_web_view_get_title(web_view);
  tab->title = title ? title : "";

  if (tab->id == self->active_tab_id_) {
    self->update_title(tab->title);
  }
  self->update_tab_label(tab->id, tab->title, tab->loading);
}

void BrowserWindow::on_uri_changed(WebKitWebView *web_view,
                                   GParamSpec * /*pspec*/, gpointer user_data) {
  auto *self = static_cast<BrowserWindow *>(user_data);
  Tab *tab = self->find_tab_by_webview(web_view);
  if (!tab)
    return;

  const char *uri = webkit_web_view_get_uri(web_view);
  if (uri) {
    tab->url = uri;
    if (tab->id == self->active_tab_id_) {
      self->update_address_bar(tab->url);
    }
  }
}

gboolean BrowserWindow::on_decide_policy(WebKitWebView * /*web_view*/,
                                         WebKitPolicyDecision *decision,
                                         WebKitPolicyDecisionType type,
                                         gpointer user_data) {
  auto *self = static_cast<BrowserWindow *>(user_data);

  if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
    auto *nav_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
    WebKitNavigationAction *action =
        webkit_navigation_policy_decision_get_navigation_action(nav_decision);
    WebKitURIRequest *request = webkit_navigation_action_get_request(action);
    const char *uri = webkit_uri_request_get_uri(request);

    // Ad / tracker blocking
    if (uri &&
        (SettingsManager::instance().block_ads() ||
         SettingsManager::instance().block_trackers()) &&
        AdBlocker::instance().should_block(uri, "")) {
      webkit_policy_decision_ignore(decision);
      return TRUE;
    }
  }

  // Default: use the resource
  (void)self;
  return FALSE;
}

WebKitWebView *BrowserWindow::on_create_webview(WebKitWebView * /*web_view*/,
                                                WebKitNavigationAction *action,
                                                gpointer user_data) {
  auto *self = static_cast<BrowserWindow *>(user_data);
  WebKitURIRequest *request = webkit_navigation_action_get_request(action);
  const char *uri = webkit_uri_request_get_uri(request);
  const std::string url = uri ? uri : "openbrowser://newtab";
  self->new_tab(url);
  Tab *tab = self->active_tab();
  return tab ? tab->webview : nullptr;
}

// Menu action callbacks
void BrowserWindow::on_menu_settings(GSimpleAction *, GVariant *, gpointer ud) {
  static_cast<BrowserWindow *>(ud)->open_settings();
}
void BrowserWindow::on_menu_bookmarks(GSimpleAction *, GVariant *,
                                      gpointer ud) {
  static_cast<BrowserWindow *>(ud)->open_bookmarks();
}
void BrowserWindow::on_menu_history(GSimpleAction *, GVariant *, gpointer ud) {
  static_cast<BrowserWindow *>(ud)->open_history();
}
void BrowserWindow::on_menu_downloads(GSimpleAction *, GVariant *,
                                      gpointer ud) {
  static_cast<BrowserWindow *>(ud)->open_downloads();
}
void BrowserWindow::on_menu_new_private(GSimpleAction *, GVariant *,
                                        gpointer ud) {
  auto *self = static_cast<BrowserWindow *>(ud);
  auto *new_win = new BrowserWindow(self->app_, true);
  new_win->show();
}
void BrowserWindow::on_menu_zoom_in(GSimpleAction *, GVariant *, gpointer ud) {
  auto *self = static_cast<BrowserWindow *>(ud);
  if (Tab *tab = self->active_tab()) {
    double level = webkit_web_view_get_zoom_level(tab->webview);
    webkit_web_view_set_zoom_level(tab->webview, level + 0.1);
  }
}
void BrowserWindow::on_menu_zoom_out(GSimpleAction *, GVariant *, gpointer ud) {
  auto *self = static_cast<BrowserWindow *>(ud);
  if (Tab *tab = self->active_tab()) {
    double level = webkit_web_view_get_zoom_level(tab->webview);
    webkit_web_view_set_zoom_level(tab->webview, std::max(0.25, level - 0.1));
  }
}
void BrowserWindow::on_menu_zoom_reset(GSimpleAction *, GVariant *,
                                       gpointer ud) {
  auto *self = static_cast<BrowserWindow *>(ud);
  if (Tab *tab = self->active_tab()) {
    webkit_web_view_set_zoom_level(tab->webview, 1.0);
  }
}

// ── Theme broadcast ───────────────────────────────────────────────────────

void BrowserWindow::broadcast_to_all_tabs(const std::string &js) {
  for (const Tab &tab : tabs_) {
    if (tab.webview) {
      webkit_web_view_evaluate_javascript(tab.webview, js.c_str(), -1, nullptr,
                                          nullptr, nullptr, nullptr, nullptr);
    }
  }
}

void BrowserWindow::apply_theme_to_all_tabs(const std::string &theme) {
  current_theme_ = theme;

  // ── Apply dark/light GTK CSS to the native chrome ────────────────────
  if (!dark_css_provider_) {
    dark_css_provider_ = gtk_css_provider_new();
  }

  if (theme == "dark") {
    const std::string dark_path = find_theme_file("shell-dark.css");
    if (!dark_path.empty()) {
      GFile *file = g_file_new_for_path(dark_path.c_str());
      gtk_css_provider_load_from_file(dark_css_provider_, file);
      g_object_unref(file);
    }
    if (!dark_css_applied_) {
      gtk_style_context_add_provider_for_display(
          gdk_display_get_default(), GTK_STYLE_PROVIDER(dark_css_provider_),
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
      dark_css_applied_ = true;
    }
    gtk_widget_add_css_class(window_, "dark-mode");
  } else {
    // Load empty string to clear the dark override
    gtk_css_provider_load_from_string(dark_css_provider_, "");
    gtk_widget_remove_css_class(window_, "dark-mode");
  }

  // ── Broadcast theme to all internal page WebViews ────────────────────
  const std::string js =
      "if(document.documentElement){"
      "document.documentElement.setAttribute('data-theme','" +
      theme +
      "');"
      "try{localStorage.setItem('ob_theme','" +
      theme +
      "');}catch(e){}"
      "}";
  broadcast_to_all_tabs(js);
}

// ── Script message handler (receives messages from page JS) ──────────────

void BrowserWindow::on_script_message(WebKitUserContentManager * /*manager*/,
                                      JSCValue *value, gpointer user_data) {
  auto *self = static_cast<BrowserWindow *>(user_data);

  if (!jsc_value_is_string(value))
    return;

  char *raw = jsc_value_to_string(value);
  if (!raw)
    return;
  const std::string msg(raw);
  g_free(raw);

  // Expected format: "theme:dark" or "theme:light"
  if (msg.starts_with("theme:")) {
    const std::string theme = msg.substr(6);
    if (theme == "dark" || theme == "light") {
      self->apply_theme_to_all_tabs(theme);
    }
  }
}

} // namespace open_browser
