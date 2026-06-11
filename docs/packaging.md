# Packaging Guide

Instructions for building distributable packages of Open Browser.

---

## Debian / Ubuntu (.deb)

### Prerequisites

```bash
sudo apt-get install devscripts debhelper dh-cmake
```

### Build

```bash
cd /path/to/open-browser
./packaging/build-deb.sh
```

The `.deb` file is placed in the parent directory.

### Manual

```bash
dpkg-buildpackage -us -uc -b
ls -lh ../*.deb
```

### Install

```bash
sudo dpkg -i ../open-browser_1.0.0-1_amd64.deb
sudo apt-get install -f  # Fix any missing dependencies
```

---

## AppImage

### Prerequisites

```bash
pip3 install --user appimage-builder
```

### Build

```bash
./packaging/build-appimage.sh
```

### Run

```bash
chmod +x Open_Browser-1.0.0-x86_64.AppImage
./Open_Browser-1.0.0-x86_64.AppImage
```

AppImages are self-contained and run on any Linux distribution with GLIBC ≥ 2.35.

---

## Flatpak

### Prerequisites

```bash
# Ubuntu / Debian
sudo apt-get install flatpak flatpak-builder

# Fedora
sudo dnf install flatpak flatpak-builder

# Add Flathub (required for GNOME SDK)
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
```

### Build

```bash
./packaging/build-flatpak.sh
```

### Install locally

```bash
flatpak install --user open-browser.flatpak
flatpak run io.openbrowser.Browser
```

### Submit to Flathub

1. Fork [flathub/io.openbrowser.Browser](https://github.com/flathub)
2. Copy `packaging/flatpak/io.openbrowser.Browser.yml`
3. Update the sources to point to a tagged release archive
4. Open a PR to the Flathub repository

---

## Arch Linux (AUR)

An AUR package (`open-browser`) is maintained at:
`https://aur.archlinux.org/packages/open-browser`

Install with an AUR helper:

```bash
yay -S open-browser
# or
paru -S open-browser
```

---

## RPM (Fedora / RHEL)

An RPM spec file is planned. In the meantime, use the AppImage on RPM-based systems.

---

## Release Checklist

1. Update `CMakeLists.txt` version number.
2. Update `packaging/debian/changelog`.
3. Update `resources/io.openbrowser.Browser.metainfo.xml` with new release entry.
4. Tag the release: `git tag v1.0.0 && git push --tags`
5. Build all packages: `.deb`, `AppImage`, `Flatpak`.
6. Create GitHub Release and upload artifacts.
7. Update Flathub manifest with new archive URL and checksum.
8. Update AUR PKGBUILD.
