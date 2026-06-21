# ADR-0003 — Layered architecture: an EC core library decoupled from the GUI

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

In `rstamule-cli` the EC protocol lived in a GUI-agnostic core (`src/ec/`,
`model.rs`, `config.rs`, `worker.rs`) and the Dear ImGui front-end (`ui.rs`) was
gated behind a Cargo `gui` feature flag. The core could build and run **headless**
(`cargo build --no-default-features`) and was unit-tested without a windowing
stack. This separation made the protocol independently verifiable against a live
`amuled`.

We want the same property in the Qt/C++ port: the protocol must be testable and
buildable without pulling in Qt's GUI.

## Decision

Split the codebase into two CMake targets:

- **`ec-core`** — a static library with **no Qt Widgets / GUI dependency**. It
  contains the EC codec, client/auth, high-level queries, and the domain model
  (`Stats`, `Download`, `SearchResult`, `Server`, `SharedFile`, `ConnState`,
  `DaemonPrefs`) plus formatting helpers. It may depend on Qt **Core** (e.g.
  `QByteArray`, `QtNetwork`'s `QTcpSocket`) but never on Qt **Widgets**.
- **`amule-remote-qt`** — the GUI application, which links `ec-core` and depends
  on Qt Widgets.

`ec-core` is unit-tested (Qt Test) independently of the GUI. A small headless
harness can exercise it against a real `amuled` using env-provided credentials,
mirroring `rstamule-cli`'s headless mode.

## Consequences

- The dependency edge points one way: GUI → core, never core → GUI.
- Protocol bugs can be reproduced and regression-tested without a display.
- The model types are GUI-agnostic; presentation (colours, glyphs, table
  formatting) is layered in the GUI, while pure formatting helpers
  (`human_bytes`, `human_rate`, …) live in the core.
- Slightly more CMake/structure ceremony than a single target — accepted for the
  testability payoff.

## Alternatives considered

- **Single GUI target with everything inline.** Rejected: couples protocol
  correctness to the GUI build and makes headless testing impossible — the exact
  problem the feature-flag split solved in `rstamule-cli`.
