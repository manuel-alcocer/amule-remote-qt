# ADR-0007 — CI/CD and release pipeline

- **Status:** Accepted (supersedes the earlier "deferred" decision)
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

Initially all CI/CD was deferred so v0.1.0 development could focus on the app
itself. With the feature set complete and ready for a first release, the
maintainer asked for a pipeline. The repository is **public**, so the pipeline
must be fully open and reproducible on **GitHub Actions**.

## Decision

Three GitHub Actions workflows model the git-flow path
`devel → release/<version> → main`:

1. **`static-analysis`** (`.github/workflows/static-analysis.yml`) — on push/PR
   to `devel`. Builds, runs the test suite, and runs **Cppcheck + clang-tidy**.
   A merge to `devel` must pass this gate before a release branch is cut.
2. **`release-build`** (`.github/workflows/release-build.yml`) — on push of a
   `release/**` branch (cut from `devel` once the gate is green). Builds in
   Release mode, runs the tests, and uploads the binary as a workflow artifact
   to validate before tagging.
3. **`release`** (`.github/workflows/release.yml`) — on push of a `v*` tag (after
   the release branch is validated and merged to `main`). Builds the release
   binary and publishes a GitHub Release with it attached.

- **clang-tidy** is scoped to genuine-bug checks (`bugprone-*`,
  `clang-analyzer-*`, `performance-*`) in `.clang-tidy`, with findings treated as
  errors. Style/modernize churn is intentionally excluded.
- **Cppcheck** runs against the CMake `compile_commands.json` with the Qt
  library profile and a small in-repo suppressions list.
- CI builds against **Qt 6.8** via `install-qt-action` (the distro package is
  below the project's floor) with **GCC 14**, matching [ADR-0002](0002-qt6-cpp-cmake-linux-windows.md).
- **Versioning is manual**, starting at **0.1.0**: the release branch and the
  `v<version>` tag are created by hand. semantic-release is **not** wired up
  (the flow above is deliberately explicit); Conventional Commits are still
  followed so it could be adopted later without history rewrite.

## Consequences

- Every change reaching `devel` is statically analysed in the open; the gate is
  reproducible by anyone (`run-clang-tidy -p build` + `cppcheck --project`).
- Packaging is currently a plain Linux binary tarball. Portable packaging
  (AppImage) and a Windows build are follow-ups, tracked separately.
- The first CI run may surface analyser findings or a Qt-module/tooling tweak;
  fixes land like any other change through the same gate.

## Alternatives considered

- **semantic-release-driven versioning.** Deferred: the maintainer wants an
  explicit, human-validated `release/* → main` promotion for the first releases.
- **AppImage in the first release.** Deferred to keep the initial pipeline simple
  and reliable; the binary tarball is enough to validate v0.1.0.
