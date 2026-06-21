# ADR-0002 — Qt 6 + C++17 + CMake, targeting Linux and Windows

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

The predecessor `rstamule-cli` used Rust + Dear ImGui (winit + glutin + glow).
For amule-remote-qt we want a native-looking desktop client with first-class
widgets, packaging, and long-term maintainability on **Linux and Windows**. The
project name fixes the GUI toolkit: **Qt**.

## Decision

- **Language:** C++17.
- **GUI toolkit:** **Qt 6** (Qt Widgets for v0.1.0). Widgets give us native
  controls, model/view tables for the transfer/search lists, and mature
  cross-platform behaviour without a custom render loop.
- **Build system:** **CMake** (Qt 6's first-class build system), so the same
  tree builds on Linux and Windows.
- **Target platforms:** Linux and Windows (64-bit). macOS is not a goal for
  v0.1.0 but nothing in the design should preclude it.

## Consequences

- Replaces the immediate-mode Dear ImGui model with Qt's retained-mode
  widget/signal-slot model; the threading design ([ADR-0004](0004-worker-thread-owns-socket.md))
  is built around Qt's event loop rather than a per-frame UI poll.
- CMake targets: a core library and a GUI app ([ADR-0003](0003-decouple-ec-core-from-gui.md)).
- Qt 6 is a build/runtime dependency; Windows builds need a Qt toolchain
  (MSVC or MinGW) — packaging is deferred with the pipelines ([ADR-0007](0007-defer-ci-release-pipelines.md)).
- C++17 (not 20) keeps the compiler floor low for both platforms while still
  giving `std::optional`, structured bindings, and `std::filesystem`.

## Alternatives considered

- **Qt Quick / QML.** Rejected for v0.1.0: the UI is dense, table-heavy desktop
  tooling where Widgets' model/view is a better fit; QML shines more for
  touch/animated UIs and could be revisited if a mobile target appears.
- **qmake instead of CMake.** Rejected: CMake is the recommended Qt 6 build
  system and integrates better with cross-platform CI later.
- **Keep Rust + a Qt binding (cxx-qt/rust-qt).** Rejected: the user wants a
  straight C++ codebase; bindings add friction for little gain here.
