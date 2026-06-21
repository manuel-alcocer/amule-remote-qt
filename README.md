# aMule Remote (amule-remote-qt)

> **A [linuxarena.net](https://linuxarena.net) project.**
> Part of the **linuxarena.net** app collection — see its page at
> **<https://linuxarena.net/en/apps/amule-remote-qt/>**
> (es: <https://linuxarena.net/es/apps/amule-remote-qt/>).

A Qt 6 / C++ desktop client that remotely controls a running **aMule daemon
(`amuled`)** over its *External Connection* (EC) protocol — like `amulegui`, but
as a standalone cross-platform app. It does not join the eD2k/Kad networks
itself: the daemon downloads, and this client observes it and sends commands.

**Live transfers · advanced search (eD2k/Kad) · servers · shared files · all
daemon preferences · LED speed panel · English/Spanish UI.**

## 📖 User manual

The full user manual (with an in-depth guide to the search filtering) lives on
the linuxarena wiki:

- English — <https://linuxarena.net/en/wiki/p2p/amule-remote-qt/>
- Español — <https://linuxarena.net/es/wiki/p2p/amule-remote-qt/>

## Install

- **Linux (AppImage)** — download from the
  [latest release](https://github.com/manuel-alcocer/amule-remote-qt/releases/latest),
  `chmod +x` and run. No Qt install required.
- **Windows** — download the `windows-x64.zip` from the latest release, unzip and
  run `amule-remote-qt.exe`.
- **Arch Linux (AUR)** — `yay -S amule-remote-qt` (builds from source).

## Build from source

Requires Qt 6.5+, CMake and a C++23 compiler (GCC 14+ / MSVC 2022+).

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/app/amule-remote-qt
```

## Requirements

A reachable `amuled` with the EC interface enabled and a password set (aMule's
*Remote Controls* preferences, or `~/.aMule/remote.conf`). Default EC port:
`4712`.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

---

**aMule Remote belongs to [linuxarena.net](https://linuxarena.net)** — built and
maintained as part of the linuxarena project.
