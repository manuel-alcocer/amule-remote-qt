# ADR-0006 — Settings persistence: QSettings, password stored as MD5 only, remote.conf fallback

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

`rstamule-cli` persisted settings to `~/.config/rstamule/config.ini` (a manual
`key=value` file) and, crucially, **never stored the plaintext password** — only
its lowercase 32-char MD5 hex digest, the same form aMule keeps in
`remote.conf`'s `Password=` key. It also fell back to reading aMule's
`~/.aMule/remote.conf` `[EC]` section for default `Host`/`Port` when no config
existed. We want the same security posture, made cross-platform for Windows.

## Decision

- **Storage mechanism:** **`QSettings`** in INI format
  (`QSettings::IniFormat`), under the platform config location resolved by
  `QStandardPaths::AppConfigLocation`. Organization/app are set so the path is
  `~/.config/<app>/…` on Linux and `%APPDATA%\<app>\…` on Windows.
- **Persisted keys:** `host`, `port` (default 4712), `password_md5`, `remember`,
  `auto_connect`, `show_log`, plus UI prefs (e.g. font size, graph window).
- **Password handling:** store **only the MD5 hex digest**, never plaintext.
  - If the user types a plaintext password, hash it to MD5 before storing.
  - If the user pastes a 32-char all-hex string, treat it as an already-hashed
    digest and store it lowercased as-is.
  - When `remember` is off, clear `password_md5` before saving.
  - `auto_connect` only takes effect when a password is remembered.
- **First-run fallback:** if no app config exists, read aMule's
  `~/.aMule/remote.conf` `[EC]` section for default `Host`/`Port`; otherwise use
  built-in defaults `127.0.0.1:4712`.

## Consequences

- The plaintext password is never written to disk; the stored digest is exactly
  what the EC handshake ([ADR-0005](0005-ec-wire-format-and-auth.md)) consumes as
  `pass_md5`, so no extra transformation is needed at connect time.
- `QSettings` gives correct per-platform paths for free, replacing the manual
  `XDG_CONFIG_HOME` logic from the Rust version.
- The MD5 digest is **not** a strong secret (it is password-equivalent for EC
  auth); storing it is on par with aMule's own `remote.conf`, and `remember` is
  opt-in. We do not claim it is encrypted at rest.
- `remote.conf` parsing is a small, best-effort convenience; failure to read it
  must degrade silently to defaults.

## Alternatives considered

- **Hand-rolled INI parser (like the Rust original).** Rejected: `QSettings` is
  already a dependency via Qt Core and handles Windows paths/escaping correctly.
- **OS keychain / encrypted secret store.** Out of scope for v0.1.0; the stored
  value is only an MD5 digest and `remember` is opt-in. Could be revisited.
