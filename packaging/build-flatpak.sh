#!/usr/bin/env bash
# Build a Flatpak bundle for Open Browser

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$ROOT_DIR/flatpak-build"
REPO_DIR="$ROOT_DIR/flatpak-repo"

echo "[flatpak] Checking flatpak-builder..."
if ! command -v flatpak-builder &>/dev/null; then
    echo "[flatpak] Please install flatpak-builder:"
    echo "  sudo apt-get install flatpak-builder   # Debian/Ubuntu"
    echo "  sudo dnf install flatpak-builder       # Fedora"
    exit 1
fi

echo "[flatpak] Adding GNOME SDK remote..."
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install -y flathub org.gnome.Platform//45 org.gnome.Sdk//45 || true

echo "[flatpak] Building..."
flatpak-builder \
    --repo="$REPO_DIR" \
    --force-clean \
    "$BUILD_DIR" \
    "$SCRIPT_DIR/flatpak/io.openbrowser.Browser.yml"

echo "[flatpak] Creating bundle..."
flatpak build-bundle \
    "$REPO_DIR" \
    open-browser.flatpak \
    io.openbrowser.Browser

echo "[flatpak] Bundle created: open-browser.flatpak"
ls -lh open-browser.flatpak
