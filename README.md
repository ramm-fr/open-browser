# Open Browser

**Open Browser** — A modern, minimalist, privacy-focused web browser for Linux

Built on GTK4 and WebKitGTK, Open Browser combines a clean, distraction-free interface with strong privacy defaults. No telemetry. No tracking. Just browsing.

---

## Features

- **Clean UI** — Minimal chrome, focus on content. GTK4-native interface that respects your desktop theme.
- **Privacy by Default** — Built-in ad blocking, tracker protection, and HTTPS upgrades.
- **Tab Management** — Full tab bar with keyboard shortcuts and sleeping tabs.
- **Password Manager** — Integrated, encrypted password storage via the system keyring.
- **Bookmark Manager** — Folders, import/export, and a beautiful management UI.
- **Download Manager** — Pause, resume, and track all downloads in one place.
- **Custom New Tab Page** — Greeter, quick-access shortcuts, and recent history at a glance.
- **Settings** — Full settings UI for appearance, privacy, search, performance, and more.
- **Internal Pages** — `openbrowser://newtab`, `openbrowser://settings`, `openbrowser://history`, etc.
- **Private Mode** — Full private browsing with per-window isolation.
- **Hardware Acceleration** — GPU compositing via WebKit when available.
- **Search Engines** — Brave Search default, configurable to any engine.
- **Minimal Black/White UI** — Modern minimalist design with pure black and white colors.
- **Clean Components** — All UI elements feature clean, sharp styling including tabs, settings, search bar, and scrollbars.
- **Modern Fonts** — Inter font family for a clean, modern reading experience.
- **Full-Screen Design** — Immersive browsing experience with minimal chrome.

---

## Screenshots

![Open Browser Screenshot](assets/poster.svg)

> _Minimal black/white UI with clean, sharp design_

---

## Requirements

- Linux (x86_64 or ARM64)
- GTK 4.6+
- WebKitGTK 2.38+ (webkitgtk-6.0 or webkit2gtk-4.1)
- CMake 3.20+
- C++20 compiler (GCC 11+ or Clang 13+)
- Ninja or GNU Make

---

## Installation

### From .deb Package (Recommended for Ubuntu/Debian)

Download and install the pre-built .deb package:

```bash
wget https://github.com/ramm-fr/open-browser/releases/download/v1.1.0/open-browser_1.1.0_amd64.deb
sudo apt install ./open-browser_1.1.0_amd64.deb
```

Or build the .deb package yourself:

```bash
git clone https://github.com/ramm-fr/open-browser.git
cd open-browser
bash packaging/build-deb.sh
sudo apt install build/open-browser_1.1.0_amd64.deb
```

### From Source

---

## Build Instructions

### Ubuntu 22.04 / 24.04 / Linux Mint / Pop!_OS

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake ninja-build pkg-config \
    libgtk-4-dev libwebkit2gtk-4.1-dev \
    libsoup-3.0-dev libglib2.0-dev \
    libsecret-1-dev nlohmann-json3-dev \
    g++ git

git clone https://github.com/ramm-fr/open-browser.git
cd open-browser
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
sudo ldconfig
```

### Fedora 38+

```bash
sudo dnf install -y \
    cmake ninja-build pkg-config \
    gtk4-devel webkitgtk6.0-devel \
    libsoup3-devel glib2-devel \
    libsecret-devel nlohmann-json-devel \
    gcc-c++ git

git clone https://github.com/ramm-fr/open-browser.git
cd open-browser
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
sudo ldconfig
```

### Arch Linux / Manjaro

```bash
sudo pacman -S --needed \
    cmake ninja pkgconf \
    gtk4 webkit2gtk-4.1 \
    libsoup3 glib2 \
    libsecret nlohmann-json \
    gcc git

git clone https://github.com/ramm-fr/open-browser.git
cd open-browser
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
sudo ldconfig
```

### Manual Build

```bash
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
sudo cmake --install build
sudo ldconfig
```

---

## Uninstallation

### From .deb Package

```bash
sudo apt remove open-browser
sudo apt autoremove
```

To remove configuration files:

```bash
sudo apt purge open-browser
rm -rf ~/.config/open-browser
rm -rf ~/.local/share/open-browser
```

### From Source

```bash
# Remove installed files
sudo rm -f /usr/local/bin/open-browser
sudo rm -f /usr/local/share/applications/io.openbrowser.Browser.desktop
sudo rm -f /usr/local/share/metainfo/io.openbrowser.Browser.metainfo.xml
sudo rm -rf /usr/local/share/open-browser
sudo rm -f /usr/local/share/icons/hicolor/scalable/apps/io.openbrowser.Browser.svg
sudo rm -f /usr/local/share/icons/hicolor/256x256/apps/io.openbrowser.Browser.png
sudo rm -f /usr/local/share/icons/hicolor/128x128/apps/io.openbrowser.Browser.png
sudo rm -f /usr/local/share/icons/hicolor/64x64/apps/io.openbrowser.Browser.png
sudo rm -f /usr/local/share/icons/hicolor/48x48/apps/io.openbrowser.Browser.png
sudo rm -f /usr/local/share/icons/hicolor/32x32/apps/io.openbrowser.Browser.png

# Remove user data
rm -rf ~/.config/open-browser
rm -rf ~/.local/share/open-browser
rm -rf ~/.cache/open-browser

# Update desktop database
sudo update-desktop-database /usr/local/share/applications
sudo gtk-update-icon-cache /usr/local/share/icons/hicolor 2>/dev/null || true
```

---

## Updating

### From .deb Package

```bash
# Download the latest version
wget https://github.com/ramm-fr/open-browser/releases/latest/download/open-browser_1.1.0_amd64.deb

# Install the update using dpkg
sudo dpkg -i open-browser_1.1.0_amd64.deb

# Fix any missing dependencies
sudo apt-get install -f
```

Alternatively, if the above doesn't work:

```bash
# Download the latest version
wget https://github.com/ramm-fr/open-browser/releases/latest/download/open-browser_1.1.0_amd64.deb

# Remove old version first
sudo apt remove open-browser

# Install new version
sudo dpkg -i open-browser_1.1.0_amd64.deb
sudo apt-get install -f
```

### From Source

```bash
cd open-browser
git pull origin main
./build.sh
sudo cmake --install build
```

---

## Running

```bash
# Normal mode
open-browser

# Open with a URL
open-browser --url https://example.com

# Private mode
open-browser --private

# Show version
open-browser --version
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close tab |
| `Ctrl+L` | Focus address bar |
| `Ctrl+R` | Reload |
| `Ctrl+Shift+R` | Hard reload |
| `Alt+Left` | Back |
| `Alt+Right` | Forward |
| `Ctrl+Tab` | Next tab |
| `Ctrl+Shift+Tab` | Previous tab |
| `Ctrl+Shift+N` | New private window |
| `Ctrl+D` | Bookmark page |
| `Ctrl+H` | History |
| `Ctrl+J` | Downloads |
| `Ctrl+,` | Settings |
| `F11` | Fullscreen |

---

---

## Poster

Check out the official [Open Browser Poster](assets/poster.svg) showcasing the beautiful glassmorphism UI design.

---

## License

Copyright 2024 Open Browser Contributors

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the full guide. Pull requests are welcome!

- Report bugs via [GitHub Issues](https://github.com/ramm-fr/open-browser/issues)
- Join the discussion on [GitHub Discussions](https://github.com/ramm-fr/open-browser/discussions)
