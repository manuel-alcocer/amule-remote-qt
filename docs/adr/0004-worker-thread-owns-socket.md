# ADR-0004 — Concurrency: a worker thread owns the socket; UI talks to it via signals/slots

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

EC request/response exchanges are blocking I/O. `rstamule-cli` kept the UI
responsive by giving a **worker thread sole ownership of the socket**: the UI
pushed `Command`s over an `mpsc` channel and read an `Arc<Mutex<SharedState>>`
snapshot each frame, while the worker performed the blocking exchanges and
refreshed the snapshot once per second (with a 100 ms idle poll).

Qt provides native primitives for this pattern (`QThread`, signals/slots with
queued connections), so we adopt the same ownership model expressed in Qt terms.

## Decision

- A dedicated **worker** (a `QObject` moved onto its own `QThread`) **exclusively
  owns** the `QTcpSocket`/`EcClient`. No other thread touches the socket.
- **UI → worker:** the UI invokes worker slots (or emits signals connected with
  `Qt::QueuedConnection`) carrying commands: connect/disconnect, pause/resume/
  stop/delete a download, set priority, add `ed2k://` link, connect/disconnect
  networks, start/stop search, download a result, read/write prefs.
- **Worker → UI:** the worker **emits signals** with immutable snapshot value
  objects (`Stats`, `QVector<Download>`, `ConnState`, search progress/results,
  …). We do **not** share a mutex-guarded state object across threads; Qt's
  queued signal delivery marshals copies onto the UI thread instead.
- **Refresh cadence:** ~**1 s** periodic refresh while connected (stats, download
  queue, connection state, servers, shared files, incremental updates, and
  search progress/results when a search is active), driven by a `QTimer` living
  in the worker thread.
- **Timeouts:** blocking reads/writes use bounded timeouts; on loss/timeout the
  worker emits an error/disconnected state and returns to idle.

## Consequences

- The UI thread never blocks on I/O; it only reacts to queued signals.
- Snapshot objects must be copyable value types (and registered with
  `qRegisterMetaType` where needed) — this reinforces the GUI-agnostic model of
  [ADR-0003](0003-decouple-ec-core-from-gui.md).
- Replacing Rust's `Arc<Mutex<SharedState>>` with copied signal payloads avoids
  shared mutable state and lock contention, at the cost of copying snapshots each
  tick (cheap at this data volume and 1 Hz).
- All command handlers run serially on the one worker thread, so EC exchanges
  never interleave on the wire.

## Alternatives considered

- **Async I/O on the UI thread (no worker).** Rejected: complicates the blocking
  EC exchange logic and risks UI stalls.
- **Shared `QMutex`-guarded state object (closer to the Rust original).**
  Rejected: Qt's queued signals already give safe cross-thread delivery without
  hand-managed locking; copies are simpler and less error-prone here.
