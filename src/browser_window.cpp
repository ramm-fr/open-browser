// Open Browser — browser_window.cpp
// Complete GTK4 + WebKitGTK4 browser window implementation.

#include "browser_window.h"
#include "url_resolver.h"
#include "settings_manager.h"
#include "history_manager.h"
#include "ad_blocker.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>

namespace open_browser {

namespace {

// Path to the internal pages directory (relative to binary, or installed).
std::string find_pages_dir() {
    // Try installed location first
    const char* data_dirs = g_get_system_data_dirs() ? g_get_system_data_dirs()[0] : nullptr;
    if (data_dirs) {
        std::filesystem::path installed = std::filesystem::path(data_dirs) / "open-browser" / "pages";
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
std::string internal_to_file_uri(const std::string& url) {
    static const std::string pages_dir = find_pages_dir();

    auto strip_scheme = [](const std::string& u) -> std::string {
        const std::string prefix = "openbrowser://";
        if (u.starts_with(prefix)) return u.substr(prefix.size());
        return u;
    };

    const std::string page = strip_scheme(url);

    // Map page names to HTML files
    const std::unordered_map<std::string, std::string> page_map = {
        { "newtab",    "newtab/index.html"    },
        { "settings",  "settings/index.html"  },
        { "bookmarks", "bookmarks/index.html" },
        { "history",   "history/index.html"   },
        { "downloads", "downloads/index.html" },
        { "about",     "about/index.html"     },
    };

    auto it = page_map.find(page);
    if (it != page_map.end() && !pages_dir.empty()) {
        const std::string full_path = pages_dir + "/" + it->second;
        return "file://" + full_path;
    }

    // Fallback: render a minimal error page inline
    return "data:text/html,<html><body><h1>Page not found</h1></body></html>";
}

// Inline GTK4 CSS theme for the browser chrome.
constexpr const char* kBrowserCSS = R"CSS(
/* ── Open Browser Chrome Theme ────────────────────────────── */

window.open-browser-window {
    background-color: #f8f8f8;
}

headerbar {
    background: #ffffff;
    border-bottom: 1px solid #e0e0e0;
    box-shadow: 0 1px 3px rgba(0,0,0,0.08);
    padding: 4px 8px;
    min-height: 48px;
}

headerbar button {
    border-radius: 8px;
    border: none;
    background: transparent;
    padding: 6px;
    color: #444444;
    transition: background 120ms ease;
}

headerbar button:hover {
    background: rgba(0, 0, 0, 0.07);
}

headerbar button:active {
    background: rgba(0, 0, 0, 0.14);
}

headerbar button:disabled {
    color: #b0b0b0;
}

/* Address bar */
entry#address-bar {
    border-radius: 24px;
    border: 1.5px solid #e0e0e0;
    background: #f5f5f5;
    padding: 6px 16px;
    font-size: 14px;
    min-width: 280px;
    transition: border-color 160ms ease, background 160ms ease;
}

entry#address-bar:focus {
    border-color: #0066FF;
    background: #ffffff;
    box-shadow: 0 0 0 3px rgba(0, 102, 255, 0.12);
}

/* Tab bar */
box#tab-bar {
    background: #f0f0f0;
    border-bottom: 1px solid #e0e0e0;
    padding: 3px 4px 0 4px;
    min-height: 36px;
}

/* Outer wrapper per tab: [switch-btn][close-btn] */
box.tab-outer {
    border-radius: 8px 8px 0 0;
    margin-right: 2px;
    background: transparent;
    transition: background 120ms ease;
}

box.tab-outer.active-tab-outer {
    background: #ffffff;
    box-shadow: 0 -1px 3px rgba(0,0,0,0.08);
}

button.tab-button {
    border-radius: 8px 8px 0 0;
    border: none;
    background: transparent;
    padding: 5px 10px;
    font-size: 13px;
    color: #666666;
    min-width: 60px;
    transition: background 120ms ease, color 120ms ease;
}

button.tab-button:hover {
    background: rgba(0,0,0,0.05);
    color: #222222;
}

button.tab-button.active-tab {
    background: transparent;
    color: #111111;
    font-weight: 500;
}

button.tab-close {
    border-radius: 6px;
    border: none;
    background: transparent;
    padding: 4px 5px;
    margin: 4px 3px 0 0;
    opacity: 0;
    transition: opacity 120ms ease, background 120ms ease;
    min-width: 22px;
    min-height: 22px;
    color: #666666;
}

box.tab-outer:hover button.tab-close,
box.tab-outer.active-tab-outer button.tab-close {
    opacity: 1;
}

button.tab-close:hover {
    background: rgba(0,0,0,0.12);
    color: #111111;
}

button#new-tab-button {
    border-radius: 50%;
    min-width: 28px;
    min-height: 28px;
    padding: 0;
    font-size: 18px;
    margin-left: 4px;
}

/* Window control buttons */
button.wctl-btn {
    border-radius: 50%;
    border: none;
    padding: 3px;
    min-width: 20px;
    min-height: 20px;
    background: transparent;
    opacity: 0.6;
    transition: opacity 120ms ease, background 120ms ease;
}
button.wctl-btn:hover { opacity: 1; }
button.wctl-close:hover { background: rgba(255,59,48,0.18); color: #ff3b30; }
button.wctl-min:hover   { background: rgba(255,149,0,0.18);  color: #ff9500; }
button.wctl-max:hover   { background: rgba(40,205,65,0.18);  color: #28cd41; }

/* Ensure window controls are on the right side */
headerbar > box:last-child {
    margin-left: auto;
}

/* Private mode indicator */
window.private-mode headerbar {
    background: #1a1a2e;
    color: #ffffff;
}
window.private-mode entry#address-bar {
    background: #2a2a3e;
    border-color: #5050a0;
    color: #e0e0e0;
}
window.private-mode button.tab-button.active-tab {
    background: #2a2a3e;
    color: #e0e0e0;
}
window.private-mode box#tab-bar {
    background: #151525;
}
)CSS";

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

BrowserWindow::BrowserWindow(GtkApplication* app, bool private_mode)
    : app_(app), private_mode_(private_mode)
{
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
            gdk_display_get_default(),
            GTK_STYLE_PROVIDER(css_provider_)
        );
        g_object_unref(css_provider_);
    }
    if (webkit_settings_) g_object_unref(webkit_settings_);
    if (user_content_manager_) g_object_unref(user_content_manager_);
    if (dark_css_provider_) g_object_unref(dark_css_provider_);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void BrowserWindow::show() {
    gtk_window_present(GTK_WINDOW(window_));
}

void BrowserWindow::navigate(const std::string& url) {
    Tab* tab = active_tab();
    if (!tab) return;

    const std::string resolved = UrlResolver::resolve(
        url,
        SettingsManager::instance().search_engine()
    );
    tab->url = resolved;

    if (UrlResolver::is_internal_url(url)) {
        const std::string file_uri = internal_to_file_uri(url);
        webkit_web_view_load_uri(tab->webview, file_uri.c_str());
    } else {
        webkit_web_view_load_uri(tab->webview, resolved.c_str());
    }
    update_address_bar(url);
}

void BrowserWindow::new_tab(const std::string& url) {
    Tab tab;
    tab.id           = next_tab_id_++;
    tab.url          = url;
    tab.title        = "New Tab";
    tab.loading      = false;
    tab.private_mode = private_mode_;

    tab.page_widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(tab.page_widget, TRUE);
    gtk_widget_set_vexpand(tab.page_widget, TRUE);

    setup_webview_for_tab(tab);

    const std::string page_name = std::format("tab-{}", tab.id);
    gtk_stack_add_named(GTK_STACK(webview_stack_),
                        tab.page_widget,
                        page_name.c_str());

    const int new_id = tab.id;
    tabs_.push_back(std::move(tab));

    // Set active first, then navigate so active_tab() resolves correctly
    active_tab_id_ = new_id;
    gtk_stack_set_visible_child_name(GTK_STACK(webview_stack_), page_name.c_str());
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
        [tab_id](const Tab& t) { return t.id == tab_id; });
    if (it == tabs_.end()) return;

    const std::string page_name = std::format("tab-{}", tab_id);
    GtkWidget* page = gtk_stack_get_child_by_name(GTK_STACK(webview_stack_),
                                                   page_name.c_str());
    if (page) {
        gtk_stack_remove(GTK_STACK(webview_stack_), page);
    }

    // If we're closing the active tab, switch to an adjacent one first
    if (tab_id == active_tab_id_) {
        int new_active = (it == tabs_.begin()) ? std::next(it)->id : std::prev(it)->id;
        active_tab_id_ = 0;
        tabs_.erase(it);
        switch_tab(new_active);
    } else {
        tabs_.erase(it);
    }

    rebuild_tab_bar_buttons();
}

void BrowserWindow::switch_tab(int tab_id) {
    Tab* tab = find_tab(tab_id);
    if (!tab) return;

    active_tab_id_ = tab_id;

    const std::string page_name = std::format("tab-{}", tab_id);
    gtk_stack_set_visible_child_name(GTK_STACK(webview_stack_), page_name.c_str());

    update_address_bar(tab->url);
    update_title(tab->title);
    update_navigation_buttons();
    rebuild_tab_bar_buttons();
}

void BrowserWindow::go_back() {
    Tab* tab = active_tab();
    if (tab && webkit_web_view_can_go_back(tab->webview)) {
        webkit_web_view_go_back(tab->webview);
    }
}

void BrowserWindow::go_forward() {
    Tab* tab = active_tab();
    if (tab && webkit_web_view_can_go_forward(tab->webview)) {
        webkit_web_view_go_forward(tab->webview);
    }
}

void BrowserWindow::reload() {
    Tab* tab = active_tab();
    if (tab) webkit_web_view_reload(tab->webview);
}

void BrowserWindow::stop() {
    Tab* tab = active_tab();
    if (tab) webkit_web_view_stop_loading(tab->webview);
}

void BrowserWindow::focus_address_bar() {
    gtk_widget_grab_focus(address_bar_);
    gtk_editable_select_region(GTK_EDITABLE(address_bar_), 0, -1);
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
    // Load app icon from our bundled SVG directly — avoids system icon cache issues
    {
        const std::filesystem::path exe_dir =
            std::filesystem::canonical("/proc/self/exe").parent_path();
        const std::vector<std::string> icon_paths = {
            (exe_dir / ".." / "resources" / "icons" / "hicolor" / "scalable" / "apps" / "io.openbrowser.Browser.svg").string(),
            "/usr/share/open-browser/icons/hicolor/scalable/apps/io.openbrowser.Browser.svg",
            "/usr/share/icons/hicolor/scalable/apps/io.openbrowser.Browser.svg",
        };
        for (const auto& path : icon_paths) {
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
    GtkEventController* key_ctrl = gtk_event_controller_key_new();
    g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), this);
    gtk_widget_add_controller(window_, key_ctrl);

    // Main vertical box: [tab bar] [webview stack]
    main_vbox_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window_), main_vbox_);

    setup_header_bar();
    setup_tab_bar();

    // WebView stack
    webview_stack_ = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(webview_stack_),
                                   GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(webview_stack_), 100);
    gtk_widget_set_hexpand(webview_stack_, TRUE);
    gtk_widget_set_vexpand(webview_stack_, TRUE);
    gtk_box_append(GTK_BOX(main_vbox_), webview_stack_);

    // Open the first tab
    new_tab("openbrowser://newtab");
}

void BrowserWindow::setup_header_bar() {
    header_bar_ = gtk_header_bar_new();
    // Keep system title buttons OFF — we draw our own below
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header_bar_), FALSE);
    gtk_window_set_titlebar(GTK_WINDOW(window_), header_bar_);

    // ── Window controls: minimize / maximize / close — RIGHT side ────────
    GtkWidget* wctl_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(wctl_box, 4);

    win_min_btn_ = gtk_button_new_from_icon_name("ob-circle-minus");
    gtk_widget_add_css_class(win_min_btn_, "wctl-btn");
    gtk_widget_add_css_class(win_min_btn_, "wctl-min");
    gtk_widget_set_tooltip_text(win_min_btn_, "Minimize");
    g_signal_connect(win_min_btn_, "clicked", G_CALLBACK(on_win_minimize), this);
    gtk_box_append(GTK_BOX(wctl_box), win_min_btn_);

    win_max_btn_ = gtk_button_new_from_icon_name("ob-maximize");
    gtk_widget_add_css_class(win_max_btn_, "wctl-btn");
    gtk_widget_add_css_class(win_max_btn_, "wctl-max");
    gtk_widget_set_tooltip_text(win_max_btn_, "Maximize");
    g_signal_connect(win_max_btn_, "clicked", G_CALLBACK(on_win_maximize), this);
    gtk_box_append(GTK_BOX(wctl_box), win_max_btn_);

    win_close_btn_ = gtk_button_new_from_icon_name("ob-circle-x");
    gtk_widget_add_css_class(win_close_btn_, "wctl-btn");
    gtk_widget_add_css_class(win_close_btn_, "wctl-close");
    gtk_widget_set_tooltip_text(win_close_btn_, "Close");
    g_signal_connect(win_close_btn_, "clicked", G_CALLBACK(on_win_close), this);
    gtk_box_append(GTK_BOX(wctl_box), win_close_btn_);

    // Pack on RIGHT side — pack_end items appear right-to-left in GTK4
    // Pack order: wctl first = rightmost, then menu, then new_tab = leftmost
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar_), wctl_box);

    // ── Left nav controls: back, forward, reload ──────────────────────────
    GtkWidget* nav_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);

    back_button_ = gtk_button_new_from_icon_name("ob-arrow-left");
    gtk_widget_set_tooltip_text(back_button_, "Back (Alt+Left)");
    gtk_widget_set_sensitive(back_button_, FALSE);
    g_signal_connect(back_button_, "clicked", G_CALLBACK(on_back_clicked), this);
    gtk_box_append(GTK_BOX(nav_box), back_button_);

    forward_button_ = gtk_button_new_from_icon_name("ob-arrow-right");
    gtk_widget_set_tooltip_text(forward_button_, "Forward (Alt+Right)");
    gtk_widget_set_sensitive(forward_button_, FALSE);
    g_signal_connect(forward_button_, "clicked", G_CALLBACK(on_forward_clicked), this);
    gtk_box_append(GTK_BOX(nav_box), forward_button_);

    reload_button_ = gtk_button_new_from_icon_name("ob-refresh-cw");
    gtk_widget_set_tooltip_text(reload_button_, "Reload (Ctrl+R)");
    g_signal_connect(reload_button_, "clicked", G_CALLBACK(on_reload_clicked), this);
    gtk_box_append(GTK_BOX(nav_box), reload_button_);

    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar_), nav_box);

    // ── Centre: address bar ───────────────────────────────────────────────
    GtkWidget* addr_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_hexpand(addr_box, TRUE);

    security_icon_ = gtk_image_new_from_icon_name("ob-lock");
    gtk_image_set_pixel_size(GTK_IMAGE(security_icon_), 14);
    gtk_widget_set_opacity(security_icon_, 0.45);
    gtk_box_append(GTK_BOX(addr_box), security_icon_);

    address_bar_ = gtk_entry_new();
    gtk_widget_set_name(address_bar_, "address-bar");
    gtk_widget_set_hexpand(address_bar_, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(address_bar_), "Search or enter address");
    gtk_entry_set_input_purpose(GTK_ENTRY(address_bar_), GTK_INPUT_PURPOSE_URL);
    g_signal_connect(address_bar_, "activate", G_CALLBACK(on_address_activate), this);
    gtk_box_append(GTK_BOX(addr_box), address_bar_);

    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header_bar_), addr_box);

    // ── Right controls (pack_end — rightmost packed = rightmost shown) ─────
    // Order we want left→right: [+new tab] [☰ menu] [_ □ ✕]
    // So pack in reverse: wctl FIRST (rightmost), then menu, then new_tab

    // wctl_box already packed above as first pack_end = rightmost

    GMenu* menu = g_menu_new();
    g_menu_append(menu, "Settings",           "win.settings");
    g_menu_append(menu, "Bookmarks",          "win.bookmarks");
    g_menu_append(menu, "History",            "win.history");
    g_menu_append(menu, "Downloads",          "win.downloads");
    g_menu_append(menu, "New Private Window", "win.new-private");

    GMenu* zoom_section = g_menu_new();
    g_menu_append(zoom_section, "Zoom In",    "win.zoom-in");
    g_menu_append(zoom_section, "Zoom Out",   "win.zoom-out");
    g_menu_append(zoom_section, "Actual Size","win.zoom-reset");
    g_menu_append_section(menu, "Zoom", G_MENU_MODEL(zoom_section));
    g_object_unref(zoom_section);

    menu_button_ = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button_), "ob-menu");
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button_), G_MENU_MODEL(menu));
    g_object_unref(menu);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar_), menu_button_);

    new_tab_button_ = gtk_button_new_from_icon_name("ob-plus");
    gtk_widget_set_name(new_tab_button_, "new-tab-button");
    gtk_widget_set_tooltip_text(new_tab_button_, "New Tab (Ctrl+T)");
    g_signal_connect(new_tab_button_, "clicked", G_CALLBACK(on_new_tab_clicked), this);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar_), new_tab_button_);

    // Register window actions
    auto add_action = [this](const char* name, GCallback cb) {
        GSimpleAction* action = g_simple_action_new(name, nullptr);
        g_signal_connect(action, "activate", cb, this);
        g_action_map_add_action(G_ACTION_MAP(window_), G_ACTION(action));
        g_object_unref(action);
    };

    add_action("settings",    G_CALLBACK(on_menu_settings));
    add_action("bookmarks",   G_CALLBACK(on_menu_bookmarks));
    add_action("history",     G_CALLBACK(on_menu_history));
    add_action("downloads",   G_CALLBACK(on_menu_downloads));
    add_action("new-private", G_CALLBACK(on_menu_new_private));
    add_action("zoom-in",     G_CALLBACK(on_menu_zoom_in));
    add_action("zoom-out",    G_CALLBACK(on_menu_zoom_out));
    add_action("zoom-reset",  G_CALLBACK(on_menu_zoom_reset));
}

void BrowserWindow::setup_tab_bar() {
    // Outer box: [scrolled-tabs] [add-tab-button]
    GtkWidget* tab_bar_outer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(tab_bar_outer, TRUE);
    gtk_widget_set_name(tab_bar_outer, "tab-bar-outer");

    tab_scroll_ = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab_scroll_),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_widget_set_hexpand(tab_scroll_, TRUE);

    tab_bar_box_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(tab_bar_box_, "tab-bar");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(tab_scroll_), tab_bar_box_);

    gtk_box_append(GTK_BOX(tab_bar_outer), tab_scroll_);

    // ── Add-tab button (+ at right of tab bar) ────────────────────────────
    GtkWidget* add_tab_btn = gtk_button_new_from_icon_name("ob-plus");
    gtk_widget_set_name(add_tab_btn, "tab-add-button");
    gtk_widget_set_tooltip_text(add_tab_btn, "New Tab (Ctrl+T)");
    gtk_widget_set_valign(add_tab_btn, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(add_tab_btn, 4);
    gtk_widget_set_margin_end(add_tab_btn, 6);
    g_signal_connect(add_tab_btn, "clicked", G_CALLBACK(on_new_tab_clicked), this);
    gtk_box_append(GTK_BOX(tab_bar_outer), add_tab_btn);

    gtk_box_append(GTK_BOX(main_vbox_), tab_bar_outer);
}

void BrowserWindow::setup_webview_for_tab(Tab& tab) {
    // Create shared WebKit settings once
    if (!webkit_settings_) {
        webkit_settings_ = webkit_settings_new();
        webkit_settings_set_enable_javascript(webkit_settings_, TRUE);
        webkit_settings_set_enable_smooth_scrolling(webkit_settings_, TRUE);
        webkit_settings_set_enable_webgl(webkit_settings_, TRUE);
        webkit_settings_set_enable_media(webkit_settings_, TRUE);
        webkit_settings_set_javascript_can_open_windows_automatically(webkit_settings_, FALSE);
        webkit_settings_set_enable_developer_extras(webkit_settings_, TRUE);
        webkit_settings_set_allow_modal_dialogs(webkit_settings_, FALSE);
        webkit_settings_set_media_playback_requires_user_gesture(webkit_settings_, TRUE);

        const bool hw = SettingsManager::instance().hardware_acceleration();
        webkit_settings_set_hardware_acceleration_policy(
            webkit_settings_,
            hw ? WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS
               : WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER
        );

        const int font_size = SettingsManager::instance().font_size();
        webkit_settings_set_default_font_size(webkit_settings_,
            static_cast<guint32>(font_size));
    }

    // Create network session (once per window; private mode uses ephemeral session)
    WebKitNetworkSession* network_session = nullptr;
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
        g_signal_connect(user_content_manager_,
            "script-message-received::obBridge",
            G_CALLBACK(on_script_message), this);
    }

    // Create the WebView
    tab.webview = WEBKIT_WEB_VIEW(g_object_new(
        WEBKIT_TYPE_WEB_VIEW,
        "settings",               webkit_settings_,
        "network-session",        network_session,
        "user-content-manager",   user_content_manager_,
        nullptr
    ));
    g_object_unref(network_session);

    gtk_widget_set_hexpand(GTK_WIDGET(tab.webview), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(tab.webview), TRUE);

    // Connect signals
    g_signal_connect(tab.webview, "load-changed", G_CALLBACK(on_load_changed), this);
    g_signal_connect(tab.webview, "notify::title", G_CALLBACK(on_title_changed), this);
    g_signal_connect(tab.webview, "notify::uri",   G_CALLBACK(on_uri_changed),   this);
    g_signal_connect(tab.webview, "decide-policy", G_CALLBACK(on_decide_policy), this);
    g_signal_connect(tab.webview, "create",        G_CALLBACK(on_create_webview), this);

    // Add webview to the page container
    gtk_box_append(GTK_BOX(tab.page_widget), GTK_WIDGET(tab.webview));
    gtk_widget_set_visible(tab.page_widget, TRUE);
}

void BrowserWindow::apply_css() {
    css_provider_ = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css_provider_, kBrowserCSS);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider_),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    // Add our custom Lucide icon theme search path
    GtkIconTheme* theme = gtk_icon_theme_get_for_display(gdk_display_get_default());

    // Development path: icons/ next to the binary
    const std::filesystem::path exe_dir =
        std::filesystem::canonical("/proc/self/exe").parent_path();
    const std::string dev_icons = (exe_dir / ".." / "resources" / "icons").string();
    if (std::filesystem::exists(dev_icons)) {
        gtk_icon_theme_add_search_path(theme, dev_icons.c_str());
    }

    // Installed path
    const char* data_dirs = g_get_system_data_dirs()
        ? g_get_system_data_dirs()[0] : nullptr;
    if (data_dirs) {
        const std::string installed_icons =
            std::string(data_dirs) + "/open-browser/icons";
        gtk_icon_theme_add_search_path(theme, installed_icons.c_str());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// State helpers
// ─────────────────────────────────────────────────────────────────────────────

Tab* BrowserWindow::active_tab() {
    return find_tab(active_tab_id_);
}

const Tab* BrowserWindow::active_tab() const {
    return const_cast<BrowserWindow*>(this)->find_tab(active_tab_id_);
}

Tab* BrowserWindow::find_tab(int id) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [id](const Tab& t) { return t.id == id; });
    return it != tabs_.end() ? &(*it) : nullptr;
}

Tab* BrowserWindow::find_tab_by_webview(WebKitWebView* wv) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [wv](const Tab& t) { return t.webview == wv; });
    return it != tabs_.end() ? &(*it) : nullptr;
}

void BrowserWindow::update_navigation_buttons() {
    const Tab* tab = active_tab();
    if (!tab) {
        gtk_widget_set_sensitive(back_button_, FALSE);
        gtk_widget_set_sensitive(forward_button_, FALSE);
        return;
    }
    gtk_widget_set_sensitive(back_button_,
        webkit_web_view_can_go_back(tab->webview));
    gtk_widget_set_sensitive(forward_button_,
        webkit_web_view_can_go_forward(tab->webview));
}

void BrowserWindow::update_address_bar(const std::string& url) {
    // Strip internal scheme for display
    const std::string display = url.starts_with("openbrowser://")
        ? url : url;
    gtk_editable_set_text(GTK_EDITABLE(address_bar_), display.c_str());

    // Security indicator
    const bool secure = url.starts_with("https://") ||
                        url.starts_with("openbrowser://") ||
                        url.starts_with("file://") ||
                        url.starts_with("data:");
    if (security_icon_) {
        gtk_image_set_from_icon_name(GTK_IMAGE(security_icon_),
            secure ? "ob-lock" : "ob-lock-open");
        gtk_widget_set_opacity(security_icon_, secure ? 0.6 : 1.0);
    }
}

void BrowserWindow::update_title(const std::string& title) {
    const std::string window_title = title.empty()
        ? "Open Browser"
        : title + " — Open Browser";
    gtk_window_set_title(GTK_WINDOW(window_), window_title.c_str());
}

void BrowserWindow::update_tab_label(int tab_id,
                                      const std::string& title,
                                      bool loading) {
    // We rebuild tab buttons as a simple approach for correctness.
    // A more optimised approach would update the label in place.
    (void)tab_id; (void)title; (void)loading;
    rebuild_tab_bar_buttons();
}

void BrowserWindow::rebuild_tab_bar_buttons() {
    // Remove all current children
    GtkWidget* child = gtk_widget_get_first_child(tab_bar_box_);
    while (child) {
        GtkWidget* next = gtk_widget_get_next_sibling(child);
        gtk_box_remove(GTK_BOX(tab_bar_box_), child);
        child = next;
    }

    for (const Tab& tab : tabs_) {
        // ── Outer wrapper: [tab-btn] [close-btn] side-by-side ────────────
        GtkWidget* outer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_add_css_class(outer, "tab-outer");
        if (tab.id == active_tab_id_) {
            gtk_widget_add_css_class(outer, "active-tab-outer");
        }

        // ── Tab switch button (icon + title) ──────────────────────────────
        GtkWidget* inner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        // Favicon / spinner
        if (tab.loading) {
            GtkWidget* spinner = gtk_spinner_new();
            gtk_spinner_start(GTK_SPINNER(spinner));
            gtk_widget_set_size_request(spinner, 14, 14);
            gtk_box_append(GTK_BOX(inner), spinner);
        } else {
            GtkWidget* favicon = gtk_image_new_from_icon_name("ob-globe");
            gtk_image_set_pixel_size(GTK_IMAGE(favicon), 14);
            gtk_widget_set_opacity(favicon, 0.5);
            gtk_box_append(GTK_BOX(inner), favicon);
        }

        // Title label
        std::string label_text = tab.title.empty() ? "New Tab" : tab.title;
        if (label_text.size() > 20) label_text = label_text.substr(0, 18) + "…";
        GtkWidget* label = gtk_label_new(label_text.c_str());
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
        gtk_box_append(GTK_BOX(inner), label);

        GtkWidget* tab_btn = gtk_button_new();
        gtk_button_set_child(GTK_BUTTON(tab_btn), inner);
        gtk_widget_add_css_class(tab_btn, "tab-button");
        if (tab.id == active_tab_id_) {
            gtk_widget_add_css_class(tab_btn, "active-tab");
        }

        // Switch signal — clean, one binding only
        {
            struct D { BrowserWindow* w; int id; };
            auto* d = new D{this, tab.id};
            g_signal_connect_data(tab_btn, "clicked",
                G_CALLBACK(+[](GtkButton*, gpointer p) {
                    auto* d = static_cast<D*>(p);
                    d->w->switch_tab(d->id);
                }),
                d,
                [](gpointer p, GClosure*) { delete static_cast<D*>(p); },
                G_CONNECT_DEFAULT);
        }

        // ── Close button — separate widget, NOT inside tab_btn ───────────
        GtkWidget* close_btn = gtk_button_new_from_icon_name("ob-x");
        gtk_widget_add_css_class(close_btn, "tab-close");
        gtk_widget_set_tooltip_text(close_btn, "Close tab (Ctrl+W)");
        gtk_widget_set_valign(close_btn, GTK_ALIGN_CENTER);

        {
            struct D { BrowserWindow* w; int id; };
            auto* d = new D{this, tab.id};
            g_signal_connect_data(close_btn, "clicked",
                G_CALLBACK(+[](GtkButton*, gpointer p) {
                    auto* d = static_cast<D*>(p);
                    d->w->close_tab(d->id);
                }),
                d,
                [](gpointer p, GClosure*) { delete static_cast<D*>(p); },
                G_CONNECT_DEFAULT);
        }

        gtk_box_append(GTK_BOX(outer), tab_btn);
        gtk_box_append(GTK_BOX(outer), close_btn);
        gtk_box_append(GTK_BOX(tab_bar_box_), outer);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Signal Handlers
// ─────────────────────────────────────────────────────────────────────────────

gboolean BrowserWindow::on_key_pressed(GtkEventControllerKey* /*controller*/,
                                        guint                   keyval,
                                        guint                   /*keycode*/,
                                        GdkModifierType         state,
                                        gpointer                user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    const bool ctrl  = (state & GDK_CONTROL_MASK) != 0;
    const bool shift = (state & GDK_SHIFT_MASK) != 0;
    const bool alt   = (state & GDK_ALT_MASK) != 0;

    if (ctrl && keyval == GDK_KEY_t)       { self->new_tab();            return TRUE; }
    if (ctrl && keyval == GDK_KEY_w)       { if (auto* t = self->active_tab()) self->close_tab(t->id); return TRUE; }
    if (ctrl && keyval == GDK_KEY_l)       { self->focus_address_bar();  return TRUE; }
    if (ctrl && keyval == GDK_KEY_r)       { self->reload();             return TRUE; }
    if (ctrl && keyval == GDK_KEY_d)       { /* bookmark */ return TRUE; }
    if (ctrl && keyval == GDK_KEY_h)       { self->open_history();       return TRUE; }
    if (ctrl && keyval == GDK_KEY_j)       { self->open_downloads();     return TRUE; }
    if (ctrl && keyval == GDK_KEY_comma)   { self->open_settings();      return TRUE; }
    if (alt  && keyval == GDK_KEY_Left)    { self->go_back();            return TRUE; }
    if (alt  && keyval == GDK_KEY_Right)   { self->go_forward();         return TRUE; }
    if (ctrl && shift && keyval == GDK_KEY_N) {
        auto* new_win = new BrowserWindow(self->app_, true);
        new_win->show();
        return TRUE;
    }
    // Ctrl+Tab / Ctrl+Shift+Tab
    if (ctrl && keyval == GDK_KEY_Tab) {
        if (!self->tabs_.empty()) {
            auto it = std::find_if(self->tabs_.begin(), self->tabs_.end(),
                [self](const Tab& t) { return t.id == self->active_tab_id_; });
            if (shift) {
                if (it == self->tabs_.begin()) it = self->tabs_.end();
                --it;
            } else {
                ++it;
                if (it == self->tabs_.end()) it = self->tabs_.begin();
            }
            self->switch_tab(it->id);
        }
        return TRUE;
    }
    return FALSE;
}

void BrowserWindow::on_load_changed(WebKitWebView*    web_view,
                                     WebKitLoadEvent   load_event,
                                     gpointer          user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    Tab* tab = self->find_tab_by_webview(web_view);
    if (!tab) return;

    switch (load_event) {
        case WEBKIT_LOAD_STARTED:
            tab->loading = true;
            if (tab->id == self->active_tab_id_) {
                gtk_button_set_icon_name(GTK_BUTTON(self->reload_button_), "ob-x");
                gtk_widget_set_tooltip_text(self->reload_button_, "Stop loading");
            }
            break;

        case WEBKIT_LOAD_COMMITTED: {
            const char* uri = webkit_web_view_get_uri(web_view);
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
                gtk_button_set_icon_name(GTK_BUTTON(self->reload_button_), "ob-refresh-cw");
                gtk_widget_set_tooltip_text(self->reload_button_, "Reload (Ctrl+R)");
                self->update_navigation_buttons();
            }
            // Record history (skip internal pages)
            if (!tab->url.starts_with("openbrowser://") &&
                !tab->url.starts_with("file://") &&
                !tab->url.starts_with("data:")) {
                HistoryManager::instance().add_visit(tab->url, tab->title);
            }
            self->update_tab_label(tab->id, tab->title, false);
            break;

        default:
            break;
    }
}

void BrowserWindow::on_title_changed(WebKitWebView* web_view,
                                      GParamSpec*    /*pspec*/,
                                      gpointer       user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    Tab* tab = self->find_tab_by_webview(web_view);
    if (!tab) return;

    const char* title = webkit_web_view_get_title(web_view);
    tab->title = title ? title : "";

    if (tab->id == self->active_tab_id_) {
        self->update_title(tab->title);
    }
    self->update_tab_label(tab->id, tab->title, tab->loading);
}

void BrowserWindow::on_uri_changed(WebKitWebView* web_view,
                                    GParamSpec*    /*pspec*/,
                                    gpointer       user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    Tab* tab = self->find_tab_by_webview(web_view);
    if (!tab) return;

    const char* uri = webkit_web_view_get_uri(web_view);
    if (uri) {
        tab->url = uri;
        if (tab->id == self->active_tab_id_) {
            self->update_address_bar(tab->url);
        }
    }
}

gboolean BrowserWindow::on_decide_policy(WebKitWebView*           /*web_view*/,
                                          WebKitPolicyDecision*    decision,
                                          WebKitPolicyDecisionType type,
                                          gpointer                 user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);

    if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        auto* nav_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        WebKitNavigationAction* action =
            webkit_navigation_policy_decision_get_navigation_action(nav_decision);
        WebKitURIRequest* request =
            webkit_navigation_action_get_request(action);
        const char* uri = webkit_uri_request_get_uri(request);

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

WebKitWebView* BrowserWindow::on_create_webview(WebKitWebView*          /*web_view*/,
                                                 WebKitNavigationAction* action,
                                                 gpointer                user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    WebKitURIRequest* request = webkit_navigation_action_get_request(action);
    const char* uri = webkit_uri_request_get_uri(request);
    const std::string url = uri ? uri : "openbrowser://newtab";
    self->new_tab(url);
    Tab* tab = self->active_tab();
    return tab ? tab->webview : nullptr;
}

void BrowserWindow::on_back_clicked(GtkButton* /*button*/, gpointer user_data) {
    static_cast<BrowserWindow*>(user_data)->go_back();
}

void BrowserWindow::on_forward_clicked(GtkButton* /*button*/, gpointer user_data) {
    static_cast<BrowserWindow*>(user_data)->go_forward();
}

void BrowserWindow::on_reload_clicked(GtkButton* /*button*/, gpointer user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    Tab* tab = self->active_tab();
    if (tab && tab->loading) {
        self->stop();
    } else {
        self->reload();
    }
}

void BrowserWindow::on_address_activate(GtkEntry* entry, gpointer user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    const char* text = gtk_editable_get_text(GTK_EDITABLE(entry));
    if (text && *text) {
        self->navigate(std::string(text));
    }
}

void BrowserWindow::on_new_tab_clicked(GtkButton* /*button*/, gpointer user_data) {
    static_cast<BrowserWindow*>(user_data)->new_tab();
}

// Menu action callbacks
void BrowserWindow::on_menu_settings(GSimpleAction*, GVariant*, gpointer ud) {
    static_cast<BrowserWindow*>(ud)->open_settings();
}
void BrowserWindow::on_menu_bookmarks(GSimpleAction*, GVariant*, gpointer ud) {
    static_cast<BrowserWindow*>(ud)->open_bookmarks();
}
void BrowserWindow::on_menu_history(GSimpleAction*, GVariant*, gpointer ud) {
    static_cast<BrowserWindow*>(ud)->open_history();
}
void BrowserWindow::on_menu_downloads(GSimpleAction*, GVariant*, gpointer ud) {
    static_cast<BrowserWindow*>(ud)->open_downloads();
}
void BrowserWindow::on_menu_new_private(GSimpleAction*, GVariant*, gpointer ud) {
    auto* self = static_cast<BrowserWindow*>(ud);
    auto* new_win = new BrowserWindow(self->app_, true);
    new_win->show();
}
void BrowserWindow::on_menu_zoom_in(GSimpleAction*, GVariant*, gpointer ud) {
    auto* self = static_cast<BrowserWindow*>(ud);
    if (Tab* tab = self->active_tab()) {
        double level = webkit_web_view_get_zoom_level(tab->webview);
        webkit_web_view_set_zoom_level(tab->webview, level + 0.1);
    }
}
void BrowserWindow::on_menu_zoom_out(GSimpleAction*, GVariant*, gpointer ud) {
    auto* self = static_cast<BrowserWindow*>(ud);
    if (Tab* tab = self->active_tab()) {
        double level = webkit_web_view_get_zoom_level(tab->webview);
        webkit_web_view_set_zoom_level(tab->webview, std::max(0.25, level - 0.1));
    }
}
void BrowserWindow::on_menu_zoom_reset(GSimpleAction*, GVariant*, gpointer ud) {
    auto* self = static_cast<BrowserWindow*>(ud);
    if (Tab* tab = self->active_tab()) {
        webkit_web_view_set_zoom_level(tab->webview, 1.0);
    }
}

// ── Window control handlers ───────────────────────────────────────────────

void BrowserWindow::on_win_close(GtkButton* /*b*/, gpointer ud) {
    auto* self = static_cast<BrowserWindow*>(ud);
    gtk_window_destroy(GTK_WINDOW(self->window_));
}

void BrowserWindow::on_win_minimize(GtkButton* /*b*/, gpointer ud) {
    auto* self = static_cast<BrowserWindow*>(ud);
    gtk_window_minimize(GTK_WINDOW(self->window_));
}

void BrowserWindow::on_win_maximize(GtkButton* /*b*/, gpointer ud) {
    auto* self = static_cast<BrowserWindow*>(ud);
    if (gtk_window_is_maximized(GTK_WINDOW(self->window_))) {
        gtk_window_unmaximize(GTK_WINDOW(self->window_));
    } else {
        gtk_window_maximize(GTK_WINDOW(self->window_));
    }
}

// ── Theme broadcast ───────────────────────────────────────────────────────

void BrowserWindow::broadcast_to_all_tabs(const std::string& js) {
    for (const Tab& tab : tabs_) {
        if (tab.webview) {
            webkit_web_view_evaluate_javascript(
                tab.webview, js.c_str(), -1,
                nullptr, nullptr, nullptr, nullptr, nullptr);
        }
    }
}

void BrowserWindow::apply_theme_to_all_tabs(const std::string& theme) {
    current_theme_ = theme;

    // ── Apply dark/light GTK CSS to the native chrome ────────────────────
    static constexpr const char* kDarkCSS = R"CSS(
window.open-browser-window {
    background-color: #09090b;
}
headerbar {
    background: #18181b;
    border-bottom-color: #3f3f46;
    box-shadow: 0 1px 3px rgba(0,0,0,0.35);
}
headerbar button { color: #a1a1aa; }
headerbar button:hover { background: rgba(255,255,255,0.08); color: #fafafa; }
headerbar button:disabled { color: #52525b; }
entry#address-bar {
    background: #27272a;
    border-color: #3f3f46;
    color: #fafafa;
}
entry#address-bar:focus {
    background: #18181b;
    border-color: #60a5fa;
    box-shadow: 0 0 0 3px rgba(96,165,250,0.15);
}
box#tab-bar, box#tab-bar-outer {
    background: #09090b;
    border-bottom-color: #27272a;
}
box.tab-outer { background: transparent; }
box.tab-outer.active-tab-outer { background: #18181b; }
button.tab-button { color: #71717a; }
button.tab-button:hover { background: rgba(255,255,255,0.05); color: #d4d4d8; }
button.tab-button.active-tab { color: #fafafa; }
)CSS";

    static constexpr const char* kLightCSS = "";  // revert to base theme

    if (!dark_css_provider_) {
        dark_css_provider_ = gtk_css_provider_new();
    }

    if (theme == "dark") {
        gtk_css_provider_load_from_string(dark_css_provider_, kDarkCSS);
        if (!dark_css_applied_) {
            gtk_style_context_add_provider_for_display(
                gdk_display_get_default(),
                GTK_STYLE_PROVIDER(dark_css_provider_),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
            dark_css_applied_ = true;
        }
        gtk_widget_add_css_class(window_, "dark-mode");
    } else {
        gtk_css_provider_load_from_string(dark_css_provider_, kLightCSS);
        gtk_widget_remove_css_class(window_, "dark-mode");
    }

    // ── Broadcast theme to all internal page WebViews ────────────────────
    const std::string js =
        "if(document.documentElement){"
        "document.documentElement.setAttribute('data-theme','" + theme + "');"
        "try{localStorage.setItem('ob_theme','" + theme + "');}catch(e){}"
        "}";
    broadcast_to_all_tabs(js);
}

// ── Script message handler (receives messages from page JS) ──────────────

void BrowserWindow::on_script_message(WebKitUserContentManager* /*manager*/,
                                       JSCValue*                  value,
                                       gpointer                   user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);

    if (!jsc_value_is_string(value)) return;

    char* raw = jsc_value_to_string(value);
    if (!raw) return;
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
