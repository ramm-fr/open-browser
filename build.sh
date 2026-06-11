#!/usr/bin/env bash
# Open Browser build script
# Detects the Linux distribution and installs dependencies before building.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc)}"

# ──────────────────────────────────────────────
# Colours
# ──────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${BLUE}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; }

# ──────────────────────────────────────────────
# Distro detection
# ──────────────────────────────────────────────
detect_distro() {
    if [ -f /etc/os-release ]; then
        # shellcheck source=/dev/null
        source /etc/os-release
        echo "${ID:-unknown}"
    else
        echo "unknown"
    fi
}

detect_distro_like() {
    if [ -f /etc/os-release ]; then
        # shellcheck source=/dev/null
        source /etc/os-release
        echo "${ID_LIKE:-}"
    fi
}

# ──────────────────────────────────────────────
# Dependency installation
# ──────────────────────────────────────────────
install_deps_apt() {
    info "Installing dependencies via apt..."
    sudo apt-get update -qq
    sudo apt-get install -y \
        cmake \
        ninja-build \
        pkg-config \
        libgtk-4-dev \
        libwebkit2gtk-4.1-dev \
        libsoup-3.0-dev \
        libglib2.0-dev \
        libsecret-1-dev \
        nlohmann-json3-dev \
        libgtest-dev \
        g++ \
        git
    success "Dependencies installed."
}

install_deps_dnf() {
    info "Installing dependencies via dnf..."
    sudo dnf install -y \
        cmake \
        ninja-build \
        pkgconf-pkg-config \
        gtk4-devel \
        webkitgtk6.0-devel \
        libsoup3-devel \
        glib2-devel \
        libsecret-devel \
        nlohmann-json-devel \
        gtest-devel \
        gcc-c++ \
        git
    success "Dependencies installed."
}

install_deps_pacman() {
    info "Installing dependencies via pacman..."
    sudo pacman -S --needed --noconfirm \
        cmake \
        ninja \
        pkgconf \
        gtk4 \
        webkit2gtk-4.1 \
        libsoup3 \
        glib2 \
        libsecret \
        nlohmann-json \
        gtest \
        gcc \
        git
    success "Dependencies installed."
}

install_deps_zypper() {
    info "Installing dependencies via zypper..."
    sudo zypper install -y \
        cmake \
        ninja \
        pkg-config \
        gtk4-devel \
        webkit2gtk3-soup2-devel \
        libsoup3-devel \
        glib2-devel \
        libsecret-devel \
        nlohmann_json-devel \
        gtest \
        gcc-c++ \
        git
    success "Dependencies installed."
}

install_dependencies() {
    local distro
    distro="$(detect_distro)"
    local distro_like
    distro_like="$(detect_distro_like)"

    case "${distro}" in
        ubuntu|debian|linuxmint|pop|elementary|zorin|kali)
            install_deps_apt
            ;;
        fedora|rhel|centos|rocky|alma)
            install_deps_dnf
            ;;
        arch|manjaro|endeavouros|garuda)
            install_deps_pacman
            ;;
        opensuse*|suse)
            install_deps_zypper
            ;;
        *)
            # Try ID_LIKE fallback
            if echo "${distro_like}" | grep -qiE "debian|ubuntu"; then
                install_deps_apt
            elif echo "${distro_like}" | grep -qi "fedora"; then
                install_deps_dnf
            elif echo "${distro_like}" | grep -qi "arch"; then
                install_deps_pacman
            elif echo "${distro_like}" | grep -qi "suse"; then
                install_deps_zypper
            else
                warn "Unknown distribution '${distro}'. Skipping automatic dependency installation."
                warn "Please install: cmake, ninja, gtk4-dev, webkit2gtk-4.1-dev, libsoup3-dev, libsecret-dev, nlohmann-json-dev, g++"
            fi
            ;;
    esac
}

# ──────────────────────────────────────────────
# Build
# ──────────────────────────────────────────────
configure_build() {
    info "Configuring build (type: ${BUILD_TYPE})..."
    cmake -B "${BUILD_DIR}" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
        -DBUILD_TESTS=ON \
        "${SCRIPT_DIR}"
    success "Configuration complete."
}

run_build() {
    info "Building with ${JOBS} parallel jobs..."
    cmake --build "${BUILD_DIR}" --parallel "${JOBS}"
    success "Build complete."
}

run_tests() {
    info "Running tests..."
    if ctest --test-dir "${BUILD_DIR}" --output-on-failure -j "${JOBS}"; then
        success "All tests passed."
    else
        error "Some tests failed. Check output above."
        exit 1
    fi
}

print_artifacts() {
    echo ""
    success "Build artifacts:"
    echo "  Binary:  ${BUILD_DIR}/src/open-browser"
    echo ""
    echo "To install system-wide:"
    echo "  sudo cmake --install ${BUILD_DIR}"
    echo ""
    echo "To run directly:"
    echo "  ${BUILD_DIR}/src/open-browser"
}

# ──────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────
main() {
    echo ""
    echo "  ┌─────────────────────────────────────────┐"
    echo "  │  Open Browser — Build Script             │"
    echo "  │  Version 1.0.0                           │"
    echo "  └─────────────────────────────────────────┘"
    echo ""

    # Parse arguments
    local skip_deps=0
    local skip_tests=0
    for arg in "$@"; do
        case "${arg}" in
            --skip-deps)   skip_deps=1 ;;
            --skip-tests)  skip_tests=1 ;;
            --debug)       BUILD_TYPE="Debug" ;;
            --help|-h)
                echo "Usage: $0 [--skip-deps] [--skip-tests] [--debug]"
                exit 0
                ;;
        esac
    done

    if [ "${skip_deps}" -eq 0 ]; then
        install_dependencies
    fi

    configure_build
    run_build

    if [ "${skip_tests}" -eq 0 ]; then
        run_tests
    fi

    print_artifacts
}

main "$@"
