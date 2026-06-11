# Open Browser — Architecture

## Overview

Open Browser is a native GTK4 desktop application that embeds WebKitGTK as its
web rendering engine. The codebase is structured as a thin C++ host process that
manages the browser chrome (tabs, address bar, menus) and delegates all web
content rendering to WebKit.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Open Browser Process                               │
│                                                                             │
│  ┌──────────────┐   ┌──────────────────────────────────────────────────┐   │
│  │   main.cpp   │──▶│               BrowserWindow                      │   │
│  │  GApplication│   │  (GTK4 GtkApplicationWindow + HeaderBar)         │   │
│  └──────────────┘   │                                                  │   │
│                     │  ┌──────────┐  ┌──────────────────────────────┐  │   │
│                     │  │ Tab Bar  │  │       GtkStack               │  │   │
│                     │  │(HBox of  │  │  ┌────────────────────────┐  │  │   │
│                     │  │ buttons) │  │  │  WebKitWebView (tab 1) │  │  │   │
│                     │  └──────────┘  │  ├────────────────────────┤  │  │   │
│                     │  ┌──────────┐  │  │  WebKitWebView (tab 2) │  │  │   │
│                     │  │ Address  │  │  └────────────────────────┘  │  │   │
│                     │  │   Bar    │  └──────────────────────────────┘  │   │
│                     │  └──────────┘                                     │   │
│                     └──────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌────────────────────────────────────────────────────────────────────┐    │
│  │                       Support Singletons                           │    │
│  │  SettingsManager │ BookmarkManager │ HistoryManager │ AdBlocker   │    │
│  │  DownloadManager │ PasswordManager                                 │    │
│  └────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Component Descriptions

### `main.cpp`

The application entry point. Creates a `GtkApplication` with the app ID
`io.openbrowser.Browser`, handles command line arguments (`--private`, `--url`,
`--version`), and wires up the `activate` and `open` GApplication signals.

### `BrowserWindow`

The central UI class. Owns:

- The `GtkApplicationWindow` and `GtkHeaderBar`.
- The tab list (`std::vector<Tab>`), with each tab holding a `WebKitWebView`.
- The `GtkStack` that switches between tab content.
- The tab bar (a scrollable `HBox` of per-tab buttons, rebuilt on any tab change).
- WebKit settings shared across all tabs in the window.
- A separate `WebKitWebContext` per window (ephemeral for private mode).

Navigation is delegated to `UrlResolver::resolve()` before calling
`webkit_web_view_load_uri()`.

### `UrlResolver`

A stateless utility that maps raw user input (from the address bar) to a
navigatable URL. Resolution order:

1. Known scheme → pass through.
2. Internal `openbrowser://` scheme → pass through (converted to `file://` by `BrowserWindow`).
3. Looks like a domain → prepend `https://`.
4. Otherwise → encode as a search query with the configured search engine.

### `SettingsManager`

A singleton that persists settings to
`~/.config/open-browser/settings.json` using nlohmann/json. Provides typed
get/set with optional change callbacks. All defaults are coded in
`apply_defaults()`.

### `BookmarkManager`

Singleton bookmark store backed by
`~/.local/share/open-browser/bookmarks.json`. Supports folders, search, and
find-by-URL. Bookmark IDs are auto-incrementing integers.

### `HistoryManager`

Singleton history store backed by
`~/.local/share/open-browser/history.json`. Deduplicates by URL (increments
`visit_count`). Provides date-grouped retrieval and range clearing.

### `DownloadManager`

Singleton that wraps `WebKitDownload` objects and exposes pause/resume/cancel
operations. Progress and completion events are propagated via registered callbacks.

### `PasswordManager`

Singleton that stores credential metadata in
`~/.local/share/open-browser/passwords.json` and persists secrets in the system
keyring via **libsecret**. Falls back to XOR obfuscation if no secret service is
available.

### `AdBlocker`

Singleton that implements a simple but effective ad/tracker blocker:

- A built-in list of ~45 major tracking domains is loaded at startup.
- EasyList-compatible `||domain^` rules from on-disk filter lists are parsed at
  load time into a domain hash set and a pattern list.
- Per-request decisions are O(1) for domain lookups and O(n) for patterns (n
  is typically small).
- A per-domain whitelist can be maintained by the user.

---

## Internal Pages

Internal pages use the `openbrowser://` scheme. `BrowserWindow` translates them
to `file://` URIs pointing to the installed HTML pages under
`<data-dir>/open-browser/pages/`. Each page is self-contained HTML + CSS + JS.

| URL | File |
|---|---|
| `openbrowser://newtab` | `pages/newtab/index.html` |
| `openbrowser://settings` | `pages/settings/index.html` |
| `openbrowser://bookmarks` | `pages/bookmarks/index.html` |
| `openbrowser://history` | `pages/history/index.html` |
| `openbrowser://downloads` | `pages/downloads/index.html` |
| `openbrowser://about` | `pages/about/index.html` |

---

## Data Flow: Navigation

```
User types in address bar
        │
        ▼
on_address_activate() → navigate(input)
        │
        ▼
UrlResolver::resolve(input, search_engine)
        │
    ┌───┴────────────────┐
    │                    │
  internal             external
    │                    │
    ▼                    ▼
internal_to_file_uri()  webkit_web_view_load_uri()
        │
        ▼
webkit_web_view_load_uri(file://...pages/newtab/index.html)
```

## Data Flow: Ad Blocking

```
WebKit "decide-policy" signal
        │
        ▼
on_decide_policy() extracts URI from WebKitNavigationAction
        │
        ▼
AdBlocker::should_block(request_url, page_url)
        │
    ┌───┴────┐
    │        │
  block    allow
    │        │
    ▼        ▼
ignore    (continue)
```

---

## Threading Model

Open Browser is single-threaded on the GLib main loop. All WebKit callbacks,
GTK signal handlers, and singleton operations run on the main thread.

No background threads are used. Future optimisations (e.g., async filter list
updates) should use GLib's `GTask` or `GAsyncQueue` to stay on the main loop.

---

## File Locations

| Data | Path |
|---|---|
| Settings | `~/.config/open-browser/settings.json` |
| Bookmarks | `~/.local/share/open-browser/bookmarks.json` |
| History | `~/.local/share/open-browser/history.json` |
| Passwords (metadata) | `~/.local/share/open-browser/passwords.json` |
| Passwords (secrets) | System keyring (libsecret) |
| WebKit cache | `~/.cache/open-browser/` |
| Installed pages | `/usr/share/open-browser/pages/` |
| Filter lists | `/usr/share/open-browser/filter-lists/` |
