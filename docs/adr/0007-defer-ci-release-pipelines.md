# ADR-0007 — Defer CI/CD and release pipelines until explicitly requested

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

`CLAUDE.md` records that releases are intended to be cut with **semantic-release**
on the `devel → release/0.1.0 → main` flow. However, for the initial development
of v0.1.0 the maintainer wants to focus on the application itself and **not** set
up any automation yet. This ADR records that deferral as a conscious decision so
the absence of pipelines is not mistaken for an oversight.

## Decision

- **No CI/CD, build matrices, packaging, or release automation** are added until
  the maintainer explicitly asks for them.
- In particular, **semantic-release is not wired up yet**. The branching model
  and Conventional Commits discipline are still followed (so automation can be
  enabled later with no history rewrite), but no release tooling runs.
- The first release target remains **0.1.0**, cut manually when ready.

## Consequences

- Conventional Commits remain mandatory now, purely so semantic-release can be
  switched on later without reworking history.
- Cross-platform builds (Linux, Windows from [ADR-0002](0002-qt6-cpp-cmake-linux-windows.md))
  are done locally for the time being; no automated Windows build exists yet.
- When automation is requested, a follow-up ADR (or an update to this one) will
  record the chosen CI provider, build matrix, and packaging approach.

## Alternatives considered

- **Set up GitHub Actions + semantic-release now.** Rejected per the maintainer's
  explicit instruction to defer all pipeline work until requested.
