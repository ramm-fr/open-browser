#pragma once

#include <gtk/gtk.h>
#include <webkit/webkit.h>

#include <memory>
#include <string>
#include <vector>

namespace open_browser {

// Represents a single browser tab.
struct Tab {
    int          id;
    std::string  url;
    std::string  title;
    bool         loading      = false;
    bool         private_mode = false;
    WebKitWebView* webview    = nullptr;
    GtkWidget*   page_widget  = nullptr; // Container held in the stack
};

// The main browser window. Manages all tabs, the header bar, address bar,
// and delegates navigation to the embedded WebKitWebView instances.
class BrowserWindow {
public:
    explicit BrowserWindow(GtkApplication* app, bool private_mode = false);
    ~BrowserWindow();

    // Make the window visible.
    void show();

    // Load a URL or resolve a search query in the active tab.
    void navigate(const std::string& url);

    // Tab management.
    void new_tab(const std::string& url = "openbrowser://newtab");
    void close_tab(int tab_id);
    void switch_tab(int tab_id);

    // Navigation controls.
    void go_back();
    void go_forward();
    void reload();
    void stop();

    // UI utilities.
    void focus_address_bar();
    void toggle_private_mode();

    // Internal page launchers.
    void open_downloads();
    void open_settings();
    void open_bookmarks();
    void open_history();
    void open_about();

    // Broadcast a JS message to every open tab's WebView
    void broadcast_to_all_tabs(const std::string& js);

    // Apply theme string ("light"/"dark") to all internal pages
    void apply_theme_to_all_tabs(const std::string& theme);

    GtkWidget* get_widget() { return window_; }

private:
    // Build the complete window UI.
    void build_ui();
    void setup_header_bar();
    void setup_tab_bar();
    void setup_webview_for_tab(Tab& tab);
    void apply_css();

    // State helpers.
    Tab*       active_tab();
    const Tab* active_tab() const;
    Tab*       find_tab(int id);
    Tab*       find_tab_by_webview(WebKitWebView* wv);

    void update_navigation_buttons();
    void update_address_bar(const std::string& url);
    void update_title(const std::string& title);
    void update_tab_label(int tab_id, const std::string& title, bool loading);
    void rebuild_tab_bar_buttons();

    // Keyboard shortcuts.
    static gboolean on_key_pressed(GtkEventControllerKey* controller,
                                   guint                  keyval,
                                   guint                  keycode,
                                   GdkModifierType        state,
                                   gpointer               user_data);

    // WebKit WebView signal callbacks.
    static void on_load_changed(WebKitWebView*    web_view,
                                WebKitLoadEvent   load_event,
                                gpointer          user_data);
    static void on_title_changed(WebKitWebView*   web_view,
                                 GParamSpec*      pspec,
                                 gpointer         user_data);
    static void on_uri_changed(WebKitWebView*     web_view,
                               GParamSpec*        pspec,
                               gpointer           user_data);
    static gboolean on_decide_policy(WebKitWebView*            web_view,
                                     WebKitPolicyDecision*     decision,
                                     WebKitPolicyDecisionType  type,
                                     gpointer                  user_data);
    static WebKitWebView* on_create_webview(WebKitWebView*             web_view,
                                            WebKitNavigationAction*    action,
                                            gpointer                   user_data);

    // Header bar widget callbacks.
    static void on_back_clicked(GtkButton* button, gpointer user_data);
    static void on_forward_clicked(GtkButton* button, gpointer user_data);
    static void on_reload_clicked(GtkButton* button, gpointer user_data);
    static void on_address_activate(GtkEntry* entry, gpointer user_data);
    static void on_new_tab_clicked(GtkButton* button, gpointer user_data);

    // Window control callbacks
    static void on_win_close(GtkButton* button, gpointer user_data);
    static void on_win_minimize(GtkButton* button, gpointer user_data);
    static void on_win_maximize(GtkButton* button, gpointer user_data);

    // WebKit script message handler — receives messages from page JS
    static void on_script_message(WebKitUserContentManager* manager,
                                  JSCValue*                 value,
                                  gpointer                  user_data);

    // Menu actions.
    static void on_menu_settings(GSimpleAction* action, GVariant* param, gpointer user_data);
    static void on_menu_bookmarks(GSimpleAction* action, GVariant* param, gpointer user_data);
    static void on_menu_history(GSimpleAction* action, GVariant* param, gpointer user_data);
    static void on_menu_downloads(GSimpleAction* action, GVariant* param, gpointer user_data);
    static void on_menu_new_private(GSimpleAction* action, GVariant* param, gpointer user_data);
    static void on_menu_zoom_in(GSimpleAction* action, GVariant* param, gpointer user_data);
    static void on_menu_zoom_out(GSimpleAction* action, GVariant* param, gpointer user_data);
    static void on_menu_zoom_reset(GSimpleAction* action, GVariant* param, gpointer user_data);

    // ── Widget references ──────────────────────────────────────────────────
    GtkApplication* app_           = nullptr;
    GtkWidget*      window_        = nullptr;
    GtkWidget*      header_bar_    = nullptr;
    GtkWidget*      tab_bar_box_   = nullptr;  // HBox holding tab buttons
    GtkWidget*      tab_scroll_    = nullptr;  // ScrolledWindow around tab_bar_box_
    GtkWidget*      webview_stack_ = nullptr;  // GtkStack, one page per tab
    GtkWidget*      address_bar_   = nullptr;
    GtkWidget*      back_button_   = nullptr;
    GtkWidget*      forward_button_= nullptr;
    GtkWidget*      reload_button_ = nullptr;
    GtkWidget*      new_tab_button_= nullptr;
    GtkWidget*      menu_button_   = nullptr;
    GtkWidget*      security_icon_ = nullptr;  // Lock icon in address bar
    GtkWidget*      main_vbox_     = nullptr;  // Window root VBox
    // Window control buttons (close / minimize / maximize)
    GtkWidget*      win_close_btn_    = nullptr;
    GtkWidget*      win_min_btn_      = nullptr;
    GtkWidget*      win_max_btn_      = nullptr;

    // ── Tab state ─────────────────────────────────────────────────────────
    std::vector<Tab> tabs_;
    int              active_tab_id_ = 0;
    int              next_tab_id_   = 1;
    bool             private_mode_;

    // ── Shared WebKit objects ──────────────────────────────────────────────
    WebKitSettings*             webkit_settings_        = nullptr;
    WebKitUserContentManager*   user_content_manager_   = nullptr;

    // Current theme ("light" or "dark") applied to all internal pages
    std::string current_theme_ = "light";

    // CSS provider for our custom theme
    GtkCssProvider* css_provider_      = nullptr;
    GtkCssProvider* dark_css_provider_ = nullptr;
    bool            dark_css_applied_  = false;
};

} // namespace open_browser
