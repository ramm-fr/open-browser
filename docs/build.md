# Build Guide

Detailed instructions for building Open Browser from source on all supported distributions.

---

## Requirements

| Component | Minimum version |
|---|---|
| CMake | 3.20 |
| C++ compiler | GCC 11 or Clang 13 (C++20) |
| GTK | 4.6 |
| WebKitGTK | 2.38 (webkit2gtk-4.1) |
| libsoup | 3.0 |
| libsecret | 0.20 |
| nlohmann/json | 3.10 |
| Google Test | 1.11 (tests only) |

---

## Quick build (any distro)

```bash
./build.sh
```

The script auto-detects your distribution, installs dependencies via the
system package manager, runs CMake, and executes the tests.

---

## Ubuntu 22.04 / 24.04 / Linux Mint / Pop!_OS / Zorin

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake ninja-build pkg-config \
    libgtk-4-dev libwebkit2gtk-4.1-dev \
    libsoup-3.0-dev libglib2.0-dev \
    libsecret-1-dev nlohmann-json3-dev \
    libgtest-dev g++ git

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## Fedora 38 / 39 / 40

```bash
sudo dnf install -y \
    cmake ninja-build pkgconf-pkg-config \
    gtk4-devel webkitgtk6.0-devel \
    libsoup3-devel glib2-devel \
    libsecret-devel nlohmann-json-devel \
    gtest-devel gcc-c++ git

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## Arch Linux / Manjaro / EndeavourOS

```bash
sudo pacman -S --needed \
    cmake ninja pkgconf \
    gtk4 webkit2gtk-4.1 \
    libsoup3 glib2 \
    libsecret nlohmann-json \
    gtest gcc git

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## openSUSE Tumbleweed / Leap

```bash
sudo zypper install -y \
    cmake ninja pkg-config \
    gtk4-devel webkit2gtk3-devel \
    libsoup3-devel glib2-devel \
    libsecret-devel nlohmann_json-devel \
    gtest gcc-c++ git

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## CMake options

| Option | Default | Description |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Release` | `Release`, `Debug`, `RelWithDebInfo` |
| `CMAKE_INSTALL_PREFIX` | `/usr/local` | Installation prefix |
| `BUILD_TESTS` | `ON` | Build Google Test suite |

Example with custom prefix:

```bash
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTS=ON
```

---

## Running Tests

```bash
ctest --test-dir build --output-on-failure -j$(nproc)
```

Or run the test binary directly for more verbose output:

```bash
./build/open-browser-tests --gtest_verbose
```

---

## Installation

```bash
sudo cmake --install build
```

This installs:

- `/usr/local/bin/open-browser` — the browser executable
- `/usr/local/share/applications/io.openbrowser.Browser.desktop` — desktop entry
- `/usr/local/share/metainfo/io.openbrowser.Browser.metainfo.xml` — AppStream metadata
- `/usr/local/share/open-browser/pages/` — internal HTML pages
- `/usr/local/share/open-browser/themes/` — GTK CSS theme

To register the MIME types and desktop entry:

```bash
update-desktop-database ~/.local/share/applications
xdg-mime default io.openbrowser.Browser.desktop x-scheme-handler/http
xdg-mime default io.openbrowser.Browser.desktop x-scheme-handler/https
```

---

## Debug Build

```bash
cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --parallel
./build-debug/src/open-browser
```

The debug build enables AddressSanitizer and UBSanitizer. Set
`ASAN_OPTIONS=detect_leaks=0` if you see false positives from GTK/WebKit.

---

## Cross-compilation

Not currently supported. ARM64 builds can be produced natively on an ARM64
Linux machine or via QEMU user-mode emulation.

---

## Troubleshooting

**`webkit2gtk-4.1` not found**

Make sure you are installing the `webkit2gtk-4.1` (API 4.1) package, not
`webkit2gtk-4.0`. On Ubuntu 20.04 only the 4.0 API is available in the main
repositories; upgrade to Ubuntu 22.04 or add a PPA.

**`nlohmann/json.hpp` not found**

On Ubuntu, install `nlohmann-json3-dev`. On Fedora, install
`nlohmann-json-devel`. On Arch, install `nlohmann-json`.

**Segfault on start with hardware acceleration**

Try running with:

```bash
WEBKIT_DISABLE_COMPOSITING_MODE=1 open-browser
```

Or disable hardware acceleration in settings.
