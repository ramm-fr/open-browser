#!/usr/bin/env bash
# Build an AppImage for Open Browser
# Produces: Open_Browser-1.0.0-x86_64.AppImage

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
VERSION="1.0.0"
APPDIR="$BUILD_DIR/AppDir"
ARCH="$(uname -m)"

# ── Install linuxdeploy if missing ────────────────────────────────────────────
LINUXDEPLOY="$BUILD_DIR/linuxdeploy"
LINUXDEPLOY_PLUGIN="$BUILD_DIR/linuxdeploy-plugin-gtk"

if [ ! -f "$LINUXDEPLOY" ]; then
    echo "[appimage] Downloading linuxdeploy..."
    curl -fsSL -o "$LINUXDEPLOY" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "$LINUXDEPLOY"
fi

if [ ! -f "$LINUXDEPLOY_PLUGIN" ]; then
    echo "[appimage] Downloading linuxdeploy GTK plugin..."
    curl -fsSL -o "$LINUXDEPLOY_PLUGIN" \
        "https://raw.githubusercontent.com/linuxdeploy/linuxdeploy-plugin-gtk/master/linuxdeploy-plugin-gtk.sh"
    chmod +x "$LINUXDEPLOY_PLUGIN"
fi

# ── Build binary ──────────────────────────────────────────────────────────────
echo "[appimage] Building Open Browser..."
cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTS=OFF \
    "$ROOT_DIR"
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

# ── Stage AppDir ──────────────────────────────────────────────────────────────
echo "[appimage] Staging AppDir..."
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD_DIR"

# AppRun entry point
cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$HERE/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"
export XDG_DATA_DIRS="$HERE/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
export GDK_BACKEND="${GDK_BACKEND:-wayland,x11}"
exec "$HERE/usr/bin/open-browser" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Desktop file at AppDir root (required by AppImage spec)
cp "$APPDIR/usr/share/applications/io.openbrowser.Browser.desktop" \
   "$APPDIR/io.openbrowser.Browser.desktop"

# Create a minimal SVG icon (placeholder — replace with real icon)
mkdir -p "$APPDIR/usr/share/icons/hicolor/scalable/apps"
cat > "$APPDIR/usr/share/icons/hicolor/scalable/apps/io.openbrowser.Browser.svg" <<'SVGEOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none"
     stroke="#0066FF" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
  <circle cx="12" cy="12" r="10"/>
  <line x1="2" y1="12" x2="22" y2="12"/>
  <path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/>
</svg>
SVGEOF

# Copy icon to AppDir root (required)
cp "$APPDIR/usr/share/icons/hicolor/scalable/apps/io.openbrowser.Browser.svg" \
   "$APPDIR/io.openbrowser.Browser.svg"

# ── Build AppImage ────────────────────────────────────────────────────────────
echo "[appimage] Bundling AppImage..."
export LINUXDEPLOY_PLUGIN_GTK="$LINUXDEPLOY_PLUGIN"
export OUTPUT="$BUILD_DIR/Open_Browser-${VERSION}-${ARCH}.AppImage"
export APPIMAGE_EXTRACT_AND_RUN=1   # avoid FUSE requirement

"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --plugin gtk \
    --output appimage 2>&1 || true

# linuxdeploy writes to cwd, move if needed
if [ -f "Open_Browser-${VERSION}-${ARCH}.AppImage" ]; then
    mv "Open_Browser-${VERSION}-${ARCH}.AppImage" "$OUTPUT"
fi

if [ -f "$OUTPUT" ]; then
    echo ""
    echo "✓ AppImage built:"
    ls -lh "$OUTPUT"
    echo ""
    echo "Run with:"
    echo "  chmod +x $OUTPUT && $OUTPUT"
else
    echo ""
    echo "⚠ AppImage tool did not produce output — manual bundle at:"
    echo "  $APPDIR"
    echo "You can run the browser directly from the build:"
    echo "  $BUILD_DIR/src/open-browser"
fi
