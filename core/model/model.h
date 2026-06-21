// Domain model: the bits of amuled state this client displays. GUI-agnostic
// (ADR-0003) — presentation (colours, glyphs) lives in the GUI; only pure data
// and string formatting live here. Ported from rstamule-cli's model.rs.

#pragma once

#include <utility>
#include <vector>

#include <QString>
#include <QtTypes>

#include "ec/tag.h" // for Hash16

namespace amule {

using Hash16 = ec::Hash16;

// A snapshot of daemon-wide statistics.
struct Stats {
    quint64 ulSpeed = 0;
    quint64 dlSpeed = 0;
    quint64 ulSpeedLimit = 0;
    quint64 dlSpeedLimit = 0;
    quint64 totalSourceCount = 0;
    quint64 ed2kUsers = 0;
    quint64 kadUsers = 0;
    quint64 ed2kFiles = 0;
    quint64 kadFiles = 0;
    // Our ed2k client ID (0 if not connected). >0x00FFFFFF means High ID.
    quint32 ed2kId = 0;

    [[nodiscard]] bool isHighId() const { return ed2kId > 0x00FF'FFFF; }
};

// One entry in the download queue (a partfile).
struct Download {
    // The daemon's per-session EC id (the EC_TAG_PARTFILE container value),
    // used to match source clients to their download.
    quint32 ecid = 0;
    Hash16 hash{};
    QString name;
    quint64 sizeFull = 0;
    quint64 sizeDone = 0;
    quint64 speed = 0;
    quint8 status = 0;
    quint64 priority = 0;
    quint64 sourceCount = 0;
    quint64 sourceCountXfer = 0;
    QString ed2kLink;
    // Alternate filenames reported by sources: (filename, how many sources
    // report it), most-reported first.
    std::vector<std::pair<QString, quint64>> sourceNames;

    [[nodiscard]] float progress() const {
        if (sizeFull == 0)
            return 0.0F;
        return static_cast<float>(static_cast<double>(sizeDone) /
                                  static_cast<double>(sizeFull));
    }

    // Human label for the partfile status (EC_TAG_PARTFILE_STATUS).
    [[nodiscard]] QString statusLabel() const;
};

// A peer that is a source for one of our downloads (EC_OP_GET_UPDATE).
struct SourceClient {
    quint32 id = 0;        // client EC id (the update-map key)
    quint32 fileEcid = 0;  // EC id of the download this is a source of
    QString name;
    QString softVer;
    quint32 userIp = 0;
    quint16 userPort = 0;
    double downSpeed = 0.0;
    quint64 downloaded = 0;
    quint64 downState = 0;
    quint64 origin = 0;
    quint64 availParts = 0;
    QString remoteName;

    [[nodiscard]] QString ipString() const;     // dotted-quad of big-endian IP
    [[nodiscard]] QString stateLabel() const;   // EC_TAG_CLIENT_DOWNLOAD_STATE
    [[nodiscard]] QString originLabel() const;  // EC_TAG_CLIENT_FROM
};

// Which network a search runs on.
enum class SearchKind { Global, Kad, Local };

// EC wire value (EC_SEARCH_*).
quint64 searchKindValue(SearchKind kind);

// Parameters for a search request.
struct SearchParams {
    QString text;
    SearchKind kind = SearchKind::Global;
    QString fileType; // eD2k file-type string ("Audio", "Video", "" for any)
    quint64 minSize = 0;
    quint64 maxSize = 0;
};

// One search result returned by the daemon.
struct SearchResult {
    quint32 ecid = 0; // daemon's result id (the EC_TAG_SEARCHFILE value)
    Hash16 hash{};
    QString name;
    quint64 size = 0;
    quint64 sourceCount = 0;
    quint64 completeSourceCount = 0;
};

// An eD2k server entry.
struct Server {
    std::array<quint8, 4> ip{};
    quint16 port = 0;
    QString name;
    QString description;
    quint64 users = 0;
    quint64 maxUsers = 0;
    quint64 files = 0;
    quint64 ping = 0;
    quint8 priority = 0;
    quint8 failed = 0;
    bool isStatic = false;
    QString version;

    [[nodiscard]] QString address() const; // dotted-quad
    [[nodiscard]] QString priorityLabel() const;
};

// A file shared by the daemon.
struct SharedFile {
    Hash16 hash{};
    QString name;
    quint64 size = 0;
    quint64 requestCount = 0;
    quint64 acceptCount = 0;
    quint64 transferred = 0;
    quint64 completeSources = 0;
    QString ed2kLink;
};

// ed2k/Kad connection state, as shown in the toolbar.
struct ConnState {
    bool ed2kConnected = false;
    QString ed2kServer;
    bool kadConnected = false;
};

// A subset of the daemon's preferences (Connection + Directories).
struct DaemonPrefs {
    bool loaded = false;
    // Connection.
    quint64 maxDownload = 0; // kB/s, 0 = unlimited
    quint64 maxUpload = 0;
    quint64 slotAllocation = 0;
    quint64 tcpPort = 0;
    quint64 udpPort = 0;
    quint64 maxSources = 0;
    quint64 maxConnections = 0;
    bool networkEd2k = false;
    bool networkKad = false;
    bool autoconnect = false;
    bool reconnect = false;
    // Directories.
    QString incomingDir;
    QString tempDir;
    bool shareHidden = false;
    bool autoRescan = false;
};

// --- Human-readable formatting helpers -------------------------------------

// Format a byte count, e.g. "3.40 MiB".
QString humanBytes(quint64 n);

// Format a byte/second rate, e.g. "3.40 MiB/s".
QString humanRate(quint64 n);

// Format a plain count compactly, e.g. 38678693 -> "38.7M", 34406 -> "34.4k".
QString humanCount(quint64 n);

// Format a done/total byte pair sharing a single unit, e.g. "3.4 / 60 MiB".
QString humanSizePair(quint64 done, quint64 full);

} // namespace amule
