# ADR-0005 — EC wire format: simplest negotiated mode, big-endian, salt+MD5 auth

- **Status:** Accepted
- **Date:** 2026-06-21
- **Deciders:** Manuel Alcocer

## Context

The EC protocol supports optional zlib compression and an FSS-UTF8 number codec,
negotiated per connection. `rstamule-cli` deliberately negotiated the **simplest
mode** so the daemon replies in plain, fixed-width big-endian integers — far
easier to (de)serialize correctly and to verify on the wire. This was confirmed
end-to-end against `amuled` 3.0.0. The Qt/C++ port should port the codec
faithfully rather than redesign it.

## Decision

Port the EC codec from `rstamule-cli` to C++ with the same wire decisions:

- **Negotiated mode:** advertise **neither** `CAN_ZLIB` nor `CAN_UTF8_NUMBERS`,
  so the daemon strips compression and UTF-8 number encoding from replies. We
  still tolerate an incoming `EC_FLAG_ZLIB` defensively but never send
  compressed.
- **Protocol version:** `EC_CURRENT_PROTOCOL_VERSION = 0x0204`.
- **Framing:** 8-byte header `u32 flags (BE) | u32 length (BE)`; payload is
  `u8 opcode | u16 tag_count (BE) | tags…`. `EC_FLAG_BASE = 0x20` set on every
  packet.
- **Tags:** recursive tree. Name wire value is `(name << 1) | has_children`;
  then `u8 type`, `u32 length (BE)` covering data **plus** serialized children,
  optional `u16 subtag_count` when `has_children`, then data. Children's bytes
  and the subtag-count are charged to the parent's length.
- **Integers:** **smallest-fit**, always big-endian — `UINT8/16/32/64`
  (types 2–5) chosen by magnitude. Other tag types: `CUSTOM`(1), `STRING`(6),
  `DOUBLE`(7), `IPV4`(8), `HASH16`(9).
- **Authentication handshake:** `AUTH_REQ (0x02)` → `AUTH_SALT (0x4F)` →
  `AUTH_PASSWD (0x50)` → `AUTH_OK (0x04)` / `AUTH_FAIL (0x03)`.
  - `AUTH_REQ` advertises client name/version, protocol version `0x0204`,
    `CAN_LARGE_TAG_COUNT`, `CAN_PARTIAL_UPDATE`.
  - Password hash: with `pass_md5 = lowercase MD5 hex of the real password`
    (aMule's `remote.conf` form) and the u64 `salt`:
    `salt_str  = uppercase hex of salt (no leading zeros)`,
    `salt_hash = lowercase MD5 hex of salt_str`,
    `response  = raw 16-byte MD5(pass_md5 + salt_hash)`.
- **Opcodes/tags** are mirrored from aMule's `ECCodes.h` (stats, download queue,
  conn state, server list, shared files, prefs read/write, search start/stop/
  results/progress, partfile pause/resume/stop/delete/prio, clear-completed,
  connect/disconnect, kad start/stop, server connect/disconnect).
- **Booleans in prefs:** when **reading**, a boolean is the mere presence of an
  empty tag; when **writing**, send it as an integer `0`/`1`.
- **Detail levels:** `EC_DETAIL_CMD`, `EC_DETAIL_FULL`, `EC_DETAIL_UPDATE`,
  `EC_DETAIL_INC_UPDATE`.

The codec is covered by round-trip unit tests (encode→decode, including nested
tags), as in the original.

## Consequences

- Simple, debuggable wire format; no zlib/codec branches on the encode path.
- Tight coupling to aMule's `ECCodes.h` constants — they must be kept in sync if
  targeting newer aMule protocol versions.
- Defensive zlib-decode on input guards against a daemon that compresses anyway.
- Big-endian, smallest-fit integer handling must be reimplemented carefully in
  C++ (no silent native-endian assumptions).

## Alternatives considered

- **Negotiate zlib + FSS-UTF (like `amulegui`).** Rejected for v0.1.0: more codec
  complexity for negligible benefit at EC's data volumes; the simple mode is
  proven.
