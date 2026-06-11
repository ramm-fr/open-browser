#!/usr/bin/env bash
# Open Browser — .deb packaging script
# Produces: open-browser_1.1.0_amd64.deb

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
VERSION="1.1.0"
ARCH="$(dpkg --print-architecture)"
PKG_NAME="open-browser_${VERSION}_${ARCH}"
PKG_DIR="$BUILD_DIR/deb-staging/$PKG_NAME"

# ── Build dependencies ─────────────────────────────────────────────────────
echo "[deb] Checking build dependencies..."
sudo apt-get update -qq
sudo apt-get install -y \
    cmake ninja-build pkg-config fakeroot \
    libgtk-4-dev libwebkitgtk-6.0-dev libsoup-3.0-dev \
    libglib2.0-dev libsecret-1-dev nlohmann-json3-dev \
    libgtest-dev g++

# ── Build ──────────────────────────────────────────────────────────────────
echo "[deb] Building Open Browser v${VERSION}..."
cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTS=OFF \
    "$ROOT_DIR"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

# ── Regenerate icons ───────────────────────────────────────────────────────
echo "[deb] Generating pixel icons..."
python3 "$ROOT_DIR/resources/gen_icon.py"

# ── Stage ──────────────────────────────────────────────────────────────────
echo "[deb] Staging package tree..."
rm -rf "$PKG_DIR"
DESTDIR="$PKG_DIR" cmake --install "$BUILD_DIR"

# ── Wrapper launch script ──────────────────────────────────────────────────
# This ensures the app always launches correctly regardless of PATH or
# display server on the target system.
WRAPPER_DIR="$PKG_DIR/usr/bin"
mkdir -p "$WRAPPER_DIR"

# Replace the installed binary with a wrapper + actual binary
BINARY_NAME="open-browser-bin"
mv "$WRAPPER_DIR/open-browser" "$WRAPPER_DIR/$BINARY_NAME"

cat > "$WRAPPER_DIR/open-browser" <<'WRAPPER_EOF'
#!/bin/bash
# Open Browser launcher wrapper
# Handles display server detection and environment setup

BINARY="/usr/bin/open-browser-bin"

if [ ! -x "$BINARY" ]; then
    echo "Error: Open Browser binary not found at $BINARY" >&2
    exit 1
fi

# Detect display server
if [ -n "${WAYLAND_DISPLAY:-}" ] && [ "${XDG_SESSION_TYPE:-}" = "wayland" ]; then
    export GDK_BACKEND="${GDK_BACKEND:-wayland}"
else
    export GDK_BACKEND="${GDK_BACKEND:-x11}"
    # Ensure DISPLAY is set
    if [ -z "${DISPLAY:-}" ]; then
        export DISPLAY=":0"
    fi
fi

# Set XDG data dir so app finds its pages
export XDG_DATA_DIRS="/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

exec "$BINARY" "$@"
WRAPPER_EOF
chmod 755 "$WRAPPER_DIR/open-browser"

# ── Update desktop file with absolute path ─────────────────────────────────
# Copy the updated desktop file (uses /usr/bin/open-browser absolute path)
cp "$ROOT_DIR/resources/io.openbrowser.Browser.desktop" \
   "$PKG_DIR/usr/share/applications/io.openbrowser.Browser.desktop"

# ── DEBIAN/control ─────────────────────────────────────────────────────────
DEBIAN_DIR="$PKG_DIR/DEBIAN"
mkdir -p "$DEBIAN_DIR"

INSTALLED_SIZE=$(du -sk "$PKG_DIR/usr" | cut -f1)

cat > "$DEBIAN_DIR/control" <<EOF
Package: open-browser
Version: $VERSION
Section: web
Priority: optional
Architecture: $ARCH
Installed-Size: $INSTALLED_SIZE
Depends: libgtk-4-1 (>= 4.6), libwebkitgtk-6.0-4 (>= 2.40), libsoup-3.0-0, libsecret-1-0
Recommends: xdg-utils
Maintainer: Open Browser Contributors <hello@openbrowser.io>
Homepage: https://openbrowser.io
Description: Open Browser ${VERSION} - Modern privacy-focused web browser for Linux
 Fast and clean web browser built on GTK4 and WebKitGTK.
 Built with privacy defaults: ad blocking, tracker protection, HTTPS upgrades.
 .
 Features: tabs, dark/light mode, bookmarks, history, downloads, passwords.
EOF

# ── postinst ───────────────────────────────────────────────────────────────
cat > "$DEBIAN_DIR/postinst" <<'POSTINST_EOF'
#!/bin/sh
set -e

# Update desktop database
update-desktop-database /usr/share/applications 2>/dev/null || true

# Update icon cache
gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true

# Register as default browser (optional)
xdg-mime default io.openbrowser.Browser.desktop x-scheme-handler/http  2>/dev/null || true
xdg-mime default io.openbrowser.Browser.desktop x-scheme-handler/https 2>/dev/null || true

echo "Open Browser installed successfully."
echo "Launch with: open-browser"
POSTINST_EOF
chmod 755 "$DEBIAN_DIR/postinst"

# ── postrm ─────────────────────────────────────────────────────────────────
cat > "$DEBIAN_DIR/postrm" <<'POSTRM_EOF'
#!/bin/sh
set -e
update-desktop-database /usr/share/applications 2>/dev/null || true
gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
POSTRM_EOF
chmod 755 "$DEBIAN_DIR/postrm"

# ── Build .deb ─────────────────────────────────────────────────────────────
echo "[deb] Building .deb package..."
fakeroot dpkg-deb --build "$PKG_DIR" "$BUILD_DIR/${PKG_NAME}.deb"

# ── Summary ────────────────────────────────────────────────────────────────
DEB_PATH="$BUILD_DIR/${PKG_NAME}.deb"
DEB_SIZE=$(du -sh "$DEB_PATH" | cut -f1)

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ✓  Open Browser v${VERSION} package ready!"
echo "  Size: ${DEB_SIZE}"
echo "  Path: ${DEB_PATH}"
echo ""
echo "  Install:"
echo "    sudo apt install ${DEB_PATH}"
echo ""
echo "  Or double-click the .deb file in your file manager."
echo ""
echo "  Run after install:"
echo "    open-browser"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
