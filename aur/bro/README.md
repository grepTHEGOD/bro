# AUR Package for bro

This directory contains the Arch User Repository (AUR) package for `bro`.

## Installation

To install from AUR:

```bash
# Using yay (recommended)
yay -S bro

# Using makepkg
git clone https://aur.archlinux.org/bro.git
cd bro
makepkg -si
```

## Package Contents

- `PKGBUILD` - Package build script
- `.SRCINFO` - Source information for AUR helpers (generated via `makepkg --printsrcinfo`)

## Dependencies

- zlib
- json-c
- ncurses
- libcurl

## Updating the Package

When updating the package version:

1. Update `pkgver` in PKGBUILD
2. Regenerate `.SRCINFO` with:
   ```bash
   makepkg --printsrcinfo > .SRCINFO
   ```

## Submitting to AUR

1. Clone the AUR repository:
   ```bash
   git clone ssh://aur@aur.archlinux.org/bro.git
   ```

2. Copy the files from this directory to the cloned repository

3. Push to AUR:
   ```bash
   git add .
   git commit -m "Update to version X.Y.Z"
   git push
   ```
