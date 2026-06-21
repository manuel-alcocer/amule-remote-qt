#include "ec/queries.h"

#include <algorithm>
#include <variant>

#include "ec/codes.h"

using namespace amule::ec;

namespace amule {
namespace {

// Send a request and discard the reply, propagating any transport error.
std::expected<void, EcError> sendDiscard(EcClient& client, const Packet& req) {
    auto resp = client.exchange(req);
    if (!resp)
        return std::unexpected(resp.error());
    return {};
}

std::expected<void, EcError> requestDiscard(EcClient& client, quint8 opcode) {
    auto resp = client.request(opcode);
    if (!resp)
        return std::unexpected(resp.error());
    return {};
}

// A single-partfile command identified by file hash (pause/resume/stop/delete).
std::expected<void, EcError>
partfileCommand(EcClient& client, quint8 opcode, Hash16 hash) {
    return sendDiscard(client, Packet(opcode, {Tag::hash(EC_TAG_PARTFILE, hash)}));
}

// Merge the present fields of an EC_TAG_CLIENT update entry into `c`. Absent
// sub-tags keep their previously-known value (incremental update).
void mergeClient(SourceClient& c, const Tag& entry) {
    if (auto v = entry.childString(EC_TAG_CLIENT_NAME))
        c.name = *v;
    if (auto v = entry.childString(EC_TAG_CLIENT_SOFT_VER_STR))
        c.softVer = *v;
    if (auto v = entry.childString(EC_TAG_CLIENT_REMOTE_FILENAME))
        c.remoteName = *v;
    if (auto v = entry.childInt(EC_TAG_CLIENT_REQUEST_FILE))
        c.fileEcid = static_cast<quint32>(*v);
    if (auto v = entry.childInt(EC_TAG_CLIENT_USER_IP))
        c.userIp = static_cast<quint32>(*v);
    if (auto v = entry.childInt(EC_TAG_CLIENT_USER_PORT))
        c.userPort = static_cast<quint16>(*v);
    if (auto v = entry.childInt(EC_TAG_CLIENT_DOWNLOAD_TOTAL))
        c.downloaded = *v;
    if (auto v = entry.childInt(EC_TAG_CLIENT_DOWNLOAD_STATE))
        c.downState = *v;
    if (auto v = entry.childInt(EC_TAG_CLIENT_FROM))
        c.origin = *v;
    if (auto v = entry.childInt(EC_TAG_CLIENT_AVAILABLE_PARTS))
        c.availParts = *v;
    if (const Tag* t = entry.child(EC_TAG_CLIENT_DOWN_SPEED)) {
        if (const auto* d = std::get_if<double>(&t->value))
            c.downSpeed = *d;
        else if (auto v = t->asInt())
            c.downSpeed = static_cast<double>(*v);
    }
}

} // namespace

// --- Reads -----------------------------------------------------------------

std::expected<Stats, EcError> fetchStats(EcClient& client) {
    auto resp = client.request(EC_OP_STAT_REQ);
    if (!resp)
        return std::unexpected(resp.error());

    Stats s;
    for (const Tag& tag : resp->tags) {
        const quint64 v = tag.asInt().value_or(0);
        switch (tag.name) {
        case EC_TAG_STATS_UL_SPEED: s.ulSpeed = v; break;
        case EC_TAG_STATS_DL_SPEED: s.dlSpeed = v; break;
        case EC_TAG_STATS_UL_SPEED_LIMIT: s.ulSpeedLimit = v; break;
        case EC_TAG_STATS_DL_SPEED_LIMIT: s.dlSpeedLimit = v; break;
        case EC_TAG_STATS_TOTAL_SRC_COUNT: s.totalSourceCount = v; break;
        case EC_TAG_STATS_ED2K_USERS: s.ed2kUsers = v; break;
        case EC_TAG_STATS_KAD_USERS: s.kadUsers = v; break;
        case EC_TAG_STATS_ED2K_FILES: s.ed2kFiles = v; break;
        case EC_TAG_STATS_KAD_FILES: s.kadFiles = v; break;
        case EC_TAG_STATS_ED2K_ID: s.ed2kId = static_cast<quint32>(v); break;
        default: break;
        }
    }
    return s;
}

std::expected<std::vector<Download>, EcError> fetchDownloads(EcClient& client) {
    auto resp = client.exchange(
        Packet(EC_OP_GET_DLOAD_QUEUE, {Tag::integer(EC_TAG_DETAIL_LEVEL, EC_DETAIL_FULL)}));
    if (!resp)
        return std::unexpected(resp.error());

    std::vector<Download> downloads;
    for (const Tag& tag : resp->tags) {
        if (tag.name != EC_TAG_PARTFILE)
            continue;
        Download d;
        d.ecid = static_cast<quint32>(tag.asInt().value_or(0));
        if (const Tag* h = tag.child(EC_TAG_PARTFILE_HASH); h && h->asHash())
            d.hash = *h->asHash();
        else if (auto h2 = tag.asHash())
            d.hash = *h2;
        d.name = tag.childString(EC_TAG_PARTFILE_NAME).value_or(QString());
        d.sizeFull = tag.childInt(EC_TAG_PARTFILE_SIZE_FULL).value_or(0);
        d.sizeDone = tag.childInt(EC_TAG_PARTFILE_SIZE_DONE).value_or(0);
        d.speed = tag.childInt(EC_TAG_PARTFILE_SPEED).value_or(0);
        d.status = static_cast<quint8>(tag.childInt(EC_TAG_PARTFILE_STATUS).value_or(0));
        d.priority = tag.childInt(EC_TAG_PARTFILE_PRIO).value_or(0);
        d.sourceCount = tag.childInt(EC_TAG_PARTFILE_SOURCE_COUNT).value_or(0);
        d.sourceCountXfer = tag.childInt(EC_TAG_PARTFILE_SOURCE_COUNT_XFER).value_or(0);
        d.ed2kLink = tag.childString(EC_TAG_PARTFILE_ED2K_LINK).value_or(QString());

        if (const Tag* names = tag.child(EC_TAG_PARTFILE_SOURCE_NAMES)) {
            for (const Tag& entry : names->children) {
                const QString name =
                    entry.childString(EC_TAG_PARTFILE_SOURCE_NAMES).value_or(QString());
                if (name.isEmpty())
                    continue;
                const quint64 count =
                    entry.childInt(EC_TAG_PARTFILE_SOURCE_NAMES_COUNTS).value_or(0);
                d.sourceNames.emplace_back(name, count);
            }
            std::stable_sort(d.sourceNames.begin(), d.sourceNames.end(),
                             [](const auto& a, const auto& b) { return a.second > b.second; });
        }
        downloads.push_back(std::move(d));
    }
    return downloads;
}

std::expected<ConnState, EcError> fetchConnState(EcClient& client) {
    auto resp = client.exchange(
        Packet(EC_OP_GET_CONNSTATE, {Tag::integer(EC_TAG_DETAIL_LEVEL, EC_DETAIL_CMD)}));
    if (!resp)
        return std::unexpected(resp.error());

    ConnState cs;
    if (const Tag* conn = resp->tag(EC_TAG_CONNSTATE)) {
        const quint64 flags = conn->asInt().value_or(0);
        cs.ed2kConnected = (flags & 0x01) != 0;
        cs.kadConnected = (flags & 0x04) != 0;
        if (const Tag* server = conn->child(EC_TAG_SERVER))
            cs.ed2kServer = server->childString(EC_TAG_SERVER_NAME).value_or(QString());
    }
    return cs;
}

std::expected<std::vector<Server>, EcError> fetchServers(EcClient& client) {
    auto resp = client.exchange(
        Packet(EC_OP_GET_SERVER_LIST, {Tag::integer(EC_TAG_DETAIL_LEVEL, EC_DETAIL_FULL)}));
    if (!resp)
        return std::unexpected(resp.error());

    std::vector<Server> servers;
    for (const Tag& tag : resp->tags) {
        if (tag.name != EC_TAG_SERVER)
            continue;
        Server s;
        if (const auto* addr = std::get_if<Ipv4>(&tag.value)) {
            s.ip = addr->ip;
            s.port = addr->port;
        }
        s.name = tag.childString(EC_TAG_SERVER_NAME).value_or(QString());
        s.description = tag.childString(EC_TAG_SERVER_DESC).value_or(QString());
        s.users = tag.childInt(EC_TAG_SERVER_USERS).value_or(0);
        s.maxUsers = tag.childInt(EC_TAG_SERVER_USERS_MAX).value_or(0);
        s.files = tag.childInt(EC_TAG_SERVER_FILES).value_or(0);
        s.ping = tag.childInt(EC_TAG_SERVER_PING).value_or(0);
        s.priority = static_cast<quint8>(tag.childInt(EC_TAG_SERVER_PRIO).value_or(0));
        s.failed = static_cast<quint8>(tag.childInt(EC_TAG_SERVER_FAILED).value_or(0));
        s.isStatic = tag.childInt(EC_TAG_SERVER_STATIC).value_or(0) != 0;
        s.version = tag.childString(EC_TAG_SERVER_VERSION).value_or(QString());
        servers.push_back(std::move(s));
    }
    return servers;
}

std::expected<std::vector<SharedFile>, EcError> fetchSharedFiles(EcClient& client) {
    auto resp = client.exchange(
        Packet(EC_OP_GET_SHARED_FILES, {Tag::integer(EC_TAG_DETAIL_LEVEL, EC_DETAIL_FULL)}));
    if (!resp)
        return std::unexpected(resp.error());

    std::vector<SharedFile> files;
    for (const Tag& tag : resp->tags) {
        if (tag.name != EC_TAG_KNOWNFILE)
            continue;
        SharedFile f;
        f.name = tag.childString(EC_TAG_PARTFILE_NAME).value_or(QString());
        f.size = tag.childInt(EC_TAG_PARTFILE_SIZE_FULL).value_or(0);
        f.requestCount = tag.childInt(EC_TAG_KNOWNFILE_REQ_COUNT_ALL).value_or(0);
        f.acceptCount = tag.childInt(EC_TAG_KNOWNFILE_ACCEPT_COUNT).value_or(0);
        f.transferred = tag.childInt(EC_TAG_KNOWNFILE_XFERRED_ALL).value_or(0);
        f.completeSources = tag.childInt(EC_TAG_KNOWNFILE_COMPLETE_SOURCES).value_or(0);
        f.ed2kLink = tag.childString(EC_TAG_PARTFILE_ED2K_LINK).value_or(QString());
        if (const Tag* h = tag.child(EC_TAG_PARTFILE_HASH); h && h->asHash())
            f.hash = *h->asHash();
        files.push_back(std::move(f));
    }
    return files;
}

std::expected<DaemonPrefs, EcError> fetchPrefs(EcClient& client) {
    auto resp = client.exchange(Packet(
        EC_OP_GET_PREFERENCES,
        {Tag::integer(EC_TAG_SELECT_PREFS, EC_PREFS_CONNECTIONS | EC_PREFS_DIRECTORIES)}));
    if (!resp)
        return std::unexpected(resp.error());

    DaemonPrefs p;
    p.loaded = true;
    if (const Tag* conn = resp->tag(EC_TAG_PREFS_CONNECTIONS)) {
        p.maxDownload = conn->childInt(EC_TAG_CONN_MAX_DL).value_or(0);
        p.maxUpload = conn->childInt(EC_TAG_CONN_MAX_UL).value_or(0);
        p.slotAllocation = conn->childInt(EC_TAG_CONN_SLOT_ALLOCATION).value_or(0);
        p.tcpPort = conn->childInt(EC_TAG_CONN_TCP_PORT).value_or(0);
        p.udpPort = conn->childInt(EC_TAG_CONN_UDP_PORT).value_or(0);
        p.maxSources = conn->childInt(EC_TAG_CONN_MAX_FILE_SOURCES).value_or(0);
        p.maxConnections = conn->childInt(EC_TAG_CONN_MAX_CONN).value_or(0);
        // Booleans are sent as the mere presence of an empty tag.
        p.networkEd2k = conn->child(EC_TAG_NETWORK_ED2K) != nullptr;
        p.networkKad = conn->child(EC_TAG_NETWORK_KADEMLIA) != nullptr;
        p.autoconnect = conn->child(EC_TAG_CONN_AUTOCONNECT) != nullptr;
        p.reconnect = conn->child(EC_TAG_CONN_RECONNECT) != nullptr;
    }
    if (const Tag* dir = resp->tag(EC_TAG_PREFS_DIRECTORIES)) {
        p.incomingDir = dir->childString(EC_TAG_DIRECTORIES_INCOMING).value_or(QString());
        p.tempDir = dir->childString(EC_TAG_DIRECTORIES_TEMP).value_or(QString());
        p.shareHidden = dir->childInt(EC_TAG_DIRECTORIES_SHARE_HIDDEN).value_or(0) != 0;
        p.autoRescan = dir->childInt(EC_TAG_DIRECTORIES_AUTO_RESCAN).value_or(0) != 0;
    }
    return p;
}

std::expected<std::vector<SearchResult>, EcError>
fetchSearchResults(EcClient& client) {
    auto resp = client.exchange(
        Packet(EC_OP_SEARCH_RESULTS, {Tag::integer(EC_TAG_DETAIL_LEVEL, EC_DETAIL_FULL)}));
    if (!resp)
        return std::unexpected(resp.error());

    std::vector<SearchResult> results;
    for (const Tag& tag : resp->tags) {
        if (tag.name != EC_TAG_SEARCHFILE)
            continue;
        SearchResult r;
        r.ecid = static_cast<quint32>(tag.asInt().value_or(0));
        if (const Tag* h = tag.child(EC_TAG_PARTFILE_HASH); h && h->asHash())
            r.hash = *h->asHash();
        r.name = tag.childString(EC_TAG_PARTFILE_NAME).value_or(QString());
        r.size = tag.childInt(EC_TAG_PARTFILE_SIZE_FULL).value_or(0);
        r.sourceCount = tag.childInt(EC_TAG_PARTFILE_SOURCE_COUNT).value_or(0);
        r.completeSourceCount = tag.childInt(EC_TAG_PARTFILE_SOURCE_COUNT_XFER).value_or(0);
        results.push_back(std::move(r));
    }
    return results;
}

std::expected<quint32, EcError> fetchSearchProgress(EcClient& client) {
    auto resp = client.request(EC_OP_SEARCH_PROGRESS);
    if (!resp)
        return std::unexpected(resp.error());
    return static_cast<quint32>(resp->tagInt(EC_TAG_SEARCH_STATUS).value_or(0));
}

std::expected<void, EcError>
fetchClientUpdate(EcClient& client,
                  std::unordered_map<quint32, SourceClient>& clients) {
    auto resp = client.exchange(
        Packet(EC_OP_GET_UPDATE, {Tag::integer(EC_TAG_DETAIL_LEVEL, EC_DETAIL_INC_UPDATE)}));
    if (!resp)
        return std::unexpected(resp.error());

    for (const Tag& container : resp->tags) {
        if (container.name != EC_TAG_CLIENT)
            continue;
        std::vector<quint32> alive;
        alive.reserve(container.children.size());
        for (const Tag& entry : container.children) {
            const auto idOpt = entry.asInt();
            if (!idOpt)
                continue;
            const auto id = static_cast<quint32>(*idOpt);
            alive.push_back(id);
            SourceClient& c = clients[id];
            c.id = id;
            mergeClient(c, entry);
        }
        // Anything not listed this round has gone away.
        std::erase_if(clients, [&](const auto& kv) {
            return std::find(alive.begin(), alive.end(), kv.first) == alive.end();
        });
        return {};
    }
    // No client container in this reply: leave the map untouched.
    return {};
}

// --- Download commands -----------------------------------------------------

std::expected<void, EcError> pauseDownload(EcClient& client, Hash16 hash) {
    return partfileCommand(client, EC_OP_PARTFILE_PAUSE, hash);
}
std::expected<void, EcError> resumeDownload(EcClient& client, Hash16 hash) {
    return partfileCommand(client, EC_OP_PARTFILE_RESUME, hash);
}
std::expected<void, EcError> stopDownload(EcClient& client, Hash16 hash) {
    return partfileCommand(client, EC_OP_PARTFILE_STOP, hash);
}
std::expected<void, EcError> deleteDownload(EcClient& client, Hash16 hash) {
    return partfileCommand(client, EC_OP_PARTFILE_DELETE, hash);
}

std::expected<void, EcError>
setPriority(EcClient& client, Hash16 hash, quint8 prio) {
    Tag partfile = Tag::hash(EC_TAG_PARTFILE, hash);
    partfile.withChildren({Tag::integer(EC_TAG_PARTFILE_PRIO, prio)});
    return sendDiscard(client, Packet(EC_OP_PARTFILE_PRIO_SET, {partfile}));
}

std::expected<void, EcError> clearCompleted(EcClient& client) {
    return requestDiscard(client, EC_OP_CLEAR_COMPLETED);
}

std::expected<void, EcError> addEd2kLink(EcClient& client, const QString& link) {
    return sendDiscard(client, Packet(EC_OP_ADD_LINK, {Tag::string(EC_TAG_STRING, link)}));
}

// --- Search ----------------------------------------------------------------

std::expected<void, EcError>
startSearch(EcClient& client, const SearchParams& params) {
    std::vector<Tag> children = {Tag::string(EC_TAG_SEARCH_NAME, params.text),
                                 Tag::string(EC_TAG_SEARCH_FILE_TYPE, params.fileType)};
    if (params.minSize > 0)
        children.push_back(Tag::integer(EC_TAG_SEARCH_MIN_SIZE, params.minSize));
    if (params.maxSize > 0)
        children.push_back(Tag::integer(EC_TAG_SEARCH_MAX_SIZE, params.maxSize));

    Tag root = Tag::integer(EC_TAG_SEARCH_TYPE, searchKindValue(params.kind));
    root.withChildren(std::move(children));

    auto resp = client.exchange(Packet(EC_OP_SEARCH_START, {root}));
    if (!resp)
        return std::unexpected(resp.error());
    if (resp->opcode == EC_OP_FAILED)
        return std::unexpected(EcError{
            EcError::Kind::Remote,
            resp->tagString(EC_TAG_STRING).value_or(QStringLiteral("search failed"))});
    return {};
}

std::expected<void, EcError> stopSearch(EcClient& client) {
    return requestDiscard(client, EC_OP_SEARCH_STOP);
}

std::expected<void, EcError> downloadSearchResult(EcClient& client, Hash16 hash) {
    Tag item = Tag::hash(EC_TAG_SEARCHFILE, hash);
    item.withChildren({Tag::integer(EC_TAG_PARTFILE_CAT, 0)});
    return sendDiscard(client, Packet(EC_OP_DOWNLOAD_SEARCH_RESULT, {item}));
}

// --- Networks --------------------------------------------------------------

std::expected<void, EcError> connectNetworks(EcClient& client) {
    return requestDiscard(client, EC_OP_CONNECT);
}
std::expected<void, EcError> disconnectNetworks(EcClient& client) {
    return requestDiscard(client, EC_OP_DISCONNECT);
}
std::expected<void, EcError> startKad(EcClient& client) {
    return requestDiscard(client, EC_OP_KAD_START);
}
std::expected<void, EcError> stopKad(EcClient& client) {
    return requestDiscard(client, EC_OP_KAD_STOP);
}

// --- eD2k servers ----------------------------------------------------------

std::expected<void, EcError>
serverConnect(EcClient& client, std::array<quint8, 4> ip, quint16 port) {
    return sendDiscard(client,
                       Packet(EC_OP_SERVER_CONNECT, {Tag(EC_TAG_SERVER, Ipv4{ip, port})}));
}
std::expected<void, EcError> serverConnectAny(EcClient& client) {
    return requestDiscard(client, EC_OP_SERVER_CONNECT);
}
std::expected<void, EcError> serverDisconnect(EcClient& client) {
    return requestDiscard(client, EC_OP_SERVER_DISCONNECT);
}
std::expected<void, EcError>
serverRemove(EcClient& client, std::array<quint8, 4> ip, quint16 port) {
    return sendDiscard(client,
                       Packet(EC_OP_SERVER_REMOVE, {Tag(EC_TAG_SERVER, Ipv4{ip, port})}));
}
std::expected<void, EcError>
serverAdd(EcClient& client, const QString& name, const QString& address) {
    auto resp = client.exchange(Packet(EC_OP_SERVER_ADD,
                                       {Tag::string(EC_TAG_SERVER_NAME, name),
                                        Tag::string(EC_TAG_SERVER_ADDRESS, address)}));
    if (!resp)
        return std::unexpected(resp.error());
    if (resp->opcode == EC_OP_FAILED)
        return std::unexpected(EcError{
            EcError::Kind::Remote,
            resp->tagString(EC_TAG_STRING).value_or(QStringLiteral("server not added"))});
    return {};
}
std::expected<void, EcError>
serverUpdateFromUrl(EcClient& client, const QString& url) {
    return sendDiscard(client,
                       Packet(EC_OP_SERVER_UPDATE_FROM_URL, {Tag::string(EC_TAG_STRING, url)}));
}

// --- Shared files & preferences --------------------------------------------

std::expected<void, EcError> reloadSharedFiles(EcClient& client) {
    return requestDiscard(client, EC_OP_SHAREDFILES_RELOAD);
}

std::expected<void, EcError> setPrefs(EcClient& client, const DaemonPrefs& p) {
    const auto b = [](bool v) -> quint64 { return v ? 1 : 0; };

    Tag conn = Tag::empty(EC_TAG_PREFS_CONNECTIONS);
    conn.withChildren({
        Tag::integer(EC_TAG_CONN_MAX_DL, p.maxDownload),
        Tag::integer(EC_TAG_CONN_MAX_UL, p.maxUpload),
        Tag::integer(EC_TAG_CONN_SLOT_ALLOCATION, p.slotAllocation),
        Tag::integer(EC_TAG_CONN_TCP_PORT, p.tcpPort),
        Tag::integer(EC_TAG_CONN_UDP_PORT, p.udpPort),
        Tag::integer(EC_TAG_CONN_MAX_FILE_SOURCES, p.maxSources),
        Tag::integer(EC_TAG_CONN_MAX_CONN, p.maxConnections),
        Tag::integer(EC_TAG_CONN_AUTOCONNECT, b(p.autoconnect)),
        Tag::integer(EC_TAG_CONN_RECONNECT, b(p.reconnect)),
        Tag::integer(EC_TAG_NETWORK_ED2K, b(p.networkEd2k)),
        Tag::integer(EC_TAG_NETWORK_KADEMLIA, b(p.networkKad)),
    });

    Tag dir = Tag::empty(EC_TAG_PREFS_DIRECTORIES);
    dir.withChildren({
        Tag::string(EC_TAG_DIRECTORIES_INCOMING, p.incomingDir),
        Tag::string(EC_TAG_DIRECTORIES_TEMP, p.tempDir),
        Tag::integer(EC_TAG_DIRECTORIES_SHARE_HIDDEN, b(p.shareHidden)),
        Tag::integer(EC_TAG_DIRECTORIES_AUTO_RESCAN, b(p.autoRescan)),
    });

    return sendDiscard(client, Packet(EC_OP_SET_PREFERENCES, {conn, dir}));
}

} // namespace amule
