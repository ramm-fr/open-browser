# Contributing to Open Browser

See also: [CONTRIBUTING.md](../CONTRIBUTING.md) in the root for the full guide.

This document supplements the root guide with technical specifics.

---

## Setting up a Development Environment

```bash
# Clone
git clone https://github.com/openbrowser/open-browser.git
cd open-browser

# Build in debug mode with tests
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure

# Run the browser
./build/src/open-browser
```

### Recommended tools

- `clang-format` — format C++ code (`clang-format -i src/*.cpp src/*.h`)
- `clang-tidy` — lint C++ code
- `valgrind` — memory checking (alternative to ASan for some scenarios)

---

## C++ Conventions

### Object lifetimes

- Prefer stack allocation; use smart pointers for heap objects.
- GTK objects follow GObject reference counting. Use `g_object_unref()` or
  GLib's smart pointer helpers when appropriate.
- `WebKitWebView` is owned by the `GtkStack`; do not unref manually.

### Signal handlers

GTK/WebKit signal handlers must be static C functions. Capture the `this`
pointer as the `gpointer user_data` parameter:

```cpp
// In header:
static void on_load_changed(WebKitWebView*, WebKitLoadEvent, gpointer);

// When connecting:
g_signal_connect(webview, "load-changed",
    G_CALLBACK(BrowserWindow::on_load_changed), this);

// Handler:
void BrowserWindow::on_load_changed(WebKitWebView* wv,
                                     WebKitLoadEvent event,
                                     gpointer user_data) {
    auto* self = static_cast<BrowserWindow*>(user_data);
    // ...
}
```

### Adding a new setting

1. Add a default in `SettingsManager::apply_defaults()`.
2. Add a typed convenience accessor in `SettingsManager` (header + `.cpp`).
3. Add the corresponding form element in `settings/index.html`.
4. Wire it up in `settings/script.js` (`populateForm` and `collectSettings`).
5. Add a test in `tests/test_settings_manager.cpp`.

### Adding a new internal page

1. Create the page directory under `src/ui/pages/<name>/`.
2. Add `index.html`, `style.css`, and `script.js`.
3. Register the page name in the `page_map` in `browser_window.cpp`.
4. Add a launch method (e.g., `open_mypage()`) to `BrowserWindow`.
5. Wire it to a menu action.

---

## JavaScript (Internal Pages)

- ES2020+ features are acceptable (WebKit is always recent).
- No third-party JS libraries. Keep pages self-contained.
- Use `localStorage` for page-level persistence (synced with C++ settings
  where needed via the injected `window.browserSettings` API).
- All user-supplied text must be escaped before insertion into the DOM.
  Use the `escHtml()` helper present in each page's `script.js`.

---

## Tests

- All public C++ API methods should have tests.
- Use `TEST_F` with a fixture that resets singleton state in `SetUp()`.
- Name tests: `ClassName_MethodName_Condition` or `ClassName_WhatItDoes`.
- HTML/JS pages should be tested manually with WebKit's developer tools.

---

## Commit Checklist

Before submitting a PR:

- [ ] `clang-format` applied to all changed `.cpp` / `.h` files.
- [ ] All tests pass: `ctest --test-dir build --output-on-failure`.
- [ ] No new compiler warnings with `-Wall -Wextra`.
- [ ] New features have tests.
- [ ] Bug fixes have regression tests.
- [ ] Commit messages follow Conventional Commits.
- [ ] PR description explains _what_ and _why_.
