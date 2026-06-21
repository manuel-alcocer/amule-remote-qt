# CONTEXT — amule-remote-qt

> **Status: provisional.** This is a greenfield repo with no source yet. The glossary
> below captures the intended domain language so issues, ADRs, and code share one
> vocabulary from the start. Refine terms as the implementation lands; flag drift via
> `/grill-with-docs`.

## What this project is

A **Qt remote client** for **aMule**. It does not download or share files itself — it
connects to a running aMule instance over the network and lets the user observe and
control it (downloads, searches, servers, shared files) from a separate device.

Initial working assumptions (revisit before implementing):

- **Transport:** the aMule **External Connection (EC)** protocol, talking to a remote
  `amuled` daemon. The client is a controller, not a P2P node.
- **Platform:** Qt-based client (desktop first).

## Glossary

Use these terms — and only these — when naming domain concepts in issues, ADRs, tests,
and code. Avoid the synonyms listed under "not".

- **aMule** — the P2P file-sharing application being controlled. An eMule-compatible
  client for the eD2k and Kad networks.
- **amuled** — the headless aMule **daemon**. The remote process this client connects to
  and controls. _(not: "server" — that word is reserved for an eD2k server.)_
- **EC protocol** — *External Connection* protocol: aMule's binary remote-control channel.
  Authenticated by host, port, and password. The transport between this client and
  `amuled`. _(not: "API".)_
- **eD2k network** — the server-based eDonkey2000 network.
- **Kad** — the Kademlia network: aMule's serverless, DHT-based overlay. _(not: "DHT" in
  user-facing text — say "Kad".)_
- **eD2k server** — a server on the eD2k network that indexes sources. Referred to as a
  **server**. Managed as a **server list**.
- **eD2k link** — an `ed2k://|file|…|` URI identifying a file by name, size, and hash;
  the primary way to add a download.
- **Download** — an inbound transfer of a file the user wants. A file still in progress is
  a **part file**; downloads live in the **download queue** with a **priority** and a
  **status** (downloading, paused, completed, …).
- **Upload** — an outbound transfer: data this aMule instance is sending to other peers.
- **Shared file** — a completed file this aMule instance offers to the network.
- **Source** — a remote peer that has all or part of a file; downloads pull from multiple
  sources. _(not: "peer" in user-facing text — say "source".)_
- **Hash** — a file's content identifier (eD2k MD4-based hash; AICH for integrity).
  Identifies a file independent of its name.
- **Search** — a query against the eD2k network (server or global) or Kad; results can be
  added to the download queue.

## Conventions

- When code or an issue title names a concept above, use the glossary term verbatim.
- If a concept you need isn't here, that's a signal: either reconsider the wording, or
  note a real gap for `/grill-with-docs` to resolve into a new term.
- Architectural decisions (e.g. EC vs. amuleweb transport, Widgets vs. QML) belong in
  `docs/adr/` once made.
