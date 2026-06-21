# ADR-0002 — Latest Qt 6 + latest C++ (C++23) + CMake, targeting Linux and Windows

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

The predecessor `rstamule-cli` used Rust + Dear ImGui (winit + glutin + glow).
For amule-remote-qt we want a native-looking desktop client with first-class
widgets, packaging, and long-term maintainability on **Linux and Windows**. The
project name fixes the GUI toolkit: **Qt**.

## Decision

- **Language:** the **latest published C++ standard, C++23** (`set(CMAKE_CXX_STANDARD 23)`,
  `CMAKE_CXX_STANDARD_REQUIRED ON`). We always track the newest standard our
  toolchains fully support; move to C++26 once GCC/Clang/MSVC ship complete
  support.
- **GUI toolkit:** the **latest stable Qt 6** (Qt 6.9 or newer; do not pin to an
  old LTS). Qt Widgets for v0.1.0 — native controls, model/view tables for the
  transfer/search lists, and mature cross-platform behaviour without a custom
  render loop.
- **Build system:** **CMake** (Qt 6's first-class build system), minimum
  **CMake 3.21+**, so the same tree builds on Linux and Windows.
- **Target platforms:** Linux and Windows (64-bit). macOS is not a goal for
  v0.1.0 but nothing in the design should preclude it.

## Consequences

- Replaces the immediate-mode Dear ImGui model with Qt's retained-mode
  widget/signal-slot model; the threading design ([ADR-0004](0004-worker-thread-owns-socket.md))
  is built around Qt's event loop rather than a per-frame UI poll.
- CMake targets: a core library and a GUI app ([ADR-0003](0003-decouple-ec-core-from-gui.md)).
- Qt 6 is a build/runtime dependency; Windows builds need a Qt toolchain
  (MSVC or MinGW) — packaging is deferred with the pipelines ([ADR-0007](0007-defer-ci-release-pipelines.md)).
- Tracking the latest Qt and C++23 means a **recent compiler floor** (GCC 13+,
  Clang 16+, MSVC 19.36+ / VS 2022 17.6+) on both platforms. This is acceptable
  given local-only builds for now; revisit if older toolchains must be supported.
- C++23 buys `std::expected`, `std::print`, ranges improvements, and deducing
  `this` over C++17 — useful for the EC codec and error handling.

## Alternatives considered

- **Qt Quick / QML.** Rejected for v0.1.0: the UI is dense, table-heavy desktop
  tooling where Widgets' model/view is a better fit; QML shines more for
  touch/animated UIs and could be revisited if a mobile target appears.
- **qmake instead of CMake.** Rejected: CMake is the recommended Qt 6 build
  system and integrates better with cross-platform CI later.
- **Keep Rust + a Qt binding (cxx-qt/rust-qt).** Rejected: the user wants a
  straight C++ codebase; bindings add friction for little gain here.
