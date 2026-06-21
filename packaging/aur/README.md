# AUR packaging

`amule-remote-qt` for the [Arch User Repository](https://aur.archlinux.org/),
built from the GitHub release source.

- `PKGBUILD` — builds from the `v<pkgver>` release tarball with CMake/Qt 6.
- `.SRCINFO` — generated metadata (regenerate with `makepkg --printsrcinfo`).

## Local test

```sh
makepkg -f          # build + package
makepkg -si         # build + install
```

## Publish / update on the AUR

The AUR is a set of git repos at `ssh://aur@aur.archlinux.org/<pkgname>.git`,
pushed with the maintainer's SSH key (configured in your AUR account).

```sh
# first time: clone the (empty) AUR repo
git clone ssh://aur@aur.archlinux.org/amule-remote-qt.git aur-amule-remote-qt
cd aur-amule-remote-qt

# copy the updated files from this repo
cp /path/to/amule-remote-qt/packaging/aur/PKGBUILD .
cp /path/to/amule-remote-qt/packaging/aur/.SRCINFO .

git add PKGBUILD .SRCINFO
git commit -m "amule-remote-qt 0.1.1-1"
git push
```

## Releasing a new version

1. Bump `pkgver` (and reset `pkgrel=1`) in `PKGBUILD`.
2. Update `sha256sums` for the new tag tarball:
   `curl -sL "$url/archive/refs/tags/vX.Y.Z.tar.gz" | sha256sum`
3. Regenerate `.SRCINFO`: `makepkg --printsrcinfo > .SRCINFO`.
4. Commit + push to the AUR repo.
