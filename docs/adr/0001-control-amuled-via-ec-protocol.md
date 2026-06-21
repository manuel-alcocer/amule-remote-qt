# ADR-0001 — Control amuled over the EC protocol (do not reimplement eD2k/Kad)

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

amule-remote-qt is a **remote control** for aMule, not a peer-to-peer node. The
eD2k and Kademlia stacks (server handshakes, source exchange, AICH, credits,
upload queueing) are large, subtle, and already implemented by aMule itself.

aMule ships a daemon, **`amuled`**, that exposes a binary remote-control channel:
the **External Connection (EC)** protocol. The official `amulegui` and
`amulecmd` drive `amuled` exactly this way. The predecessor project
`rstamule-cli` (Rust + Dear ImGui) validated this approach end-to-end against a
real `amuled` 3.0.0.

## Decision

The client connects to a running `amuled` over the **EC protocol** and acts
purely as a controller. We **do not** implement any eD2k/Kad networking
ourselves. All file transfer, searching, and network state lives in the daemon;
the client observes snapshots and issues commands (pause/resume/cancel, add
`ed2k://` link, connect/disconnect networks, start search, download result).

The daemon is addressed by **host + EC port (default 4712) + password**.

## Consequences

- The client never touches the eD2k/Kad wire — far smaller surface, no need to
  track aMule's networking internals.
- Correctness depends on faithfully matching aMule's `ECCodes.h` opcodes/tags
  and the EC framing (see [ADR-0005](0005-ec-wire-format-and-auth.md)).
- The client is useless without a reachable `amuled`; connection lifecycle and
  error reporting are first-class concerns.
- Feature scope is bounded by what EC exposes; anything the daemon does not
  surface over EC is out of scope.
- Terminology follows `CONTEXT.md`: **amuled** is the daemon, **server** is
  reserved for an eD2k server.

## Alternatives considered

- **Reimplement eD2k/Kad natively.** Rejected: enormous effort, duplicates
  aMule, and would make this a P2P client rather than a remote control.
- **Talk to `amuleweb` over HTTP.** Rejected for v0.1.0: the EC protocol is the
  native, lower-latency control channel and the path already proven by
  `rstamule-cli`. May be revisited as an alternate transport later.
