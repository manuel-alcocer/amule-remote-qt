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
        d.partMetId =
            static_cast<quint32>(tag.childInt(EC_TAG_PARTFILE_PARTMETID).value_or(0));
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
    // Most boolean prefs are reported as the mere presence of an empty tag.
    const auto flag = [](const Tag* parent, quint16 tag) {
        return parent->child(tag) != nullptr;
    };
    // ...but the Directories category reports them as 0/1 integers instead.
    const auto intFlag = [](const Tag* parent, quint16 tag) {
        return parent->childInt(tag).value_or(0) != 0;
    };

    const quint64 selectAll =
        EC_PREFS_GENERAL | EC_PREFS_CONNECTIONS | EC_PREFS_MESSAGEFILTER |
        EC_PREFS_REMOTECONTROLS | EC_PREFS_ONLINESIG | EC_PREFS_SERVERS |
        EC_PREFS_FILES | EC_PREFS_DIRECTORIES | EC_PREFS_SECURITY |
        EC_PREFS_CORETWEAKS | EC_PREFS_KADEMLIA;
    auto resp = client.exchange(
        Packet(EC_OP_GET_PREFERENCES, {Tag::integer(EC_TAG_SELECT_PREFS, selectAll)}));
    if (!resp)
        return std::unexpected(resp.error());

    DaemonPrefs p;
    p.loaded = true;

    if (const Tag* g = resp->tag(EC_TAG_PREFS_GENERAL)) {
        p.nick = g->childString(EC_TAG_USER_NICK).value_or(QString());
        p.checkNewVersion = intFlag(g, EC_TAG_GENERAL_CHECK_NEW_VERSION);
    }
    if (const Tag* c = resp->tag(EC_TAG_PREFS_CONNECTIONS)) {
        p.downloadCapacity = c->childInt(EC_TAG_CONN_DL_CAP).value_or(0);
        p.uploadCapacity = c->childInt(EC_TAG_CONN_UL_CAP).value_or(0);
        p.maxDownload = c->childInt(EC_TAG_CONN_MAX_DL).value_or(0);
        p.maxUpload = c->childInt(EC_TAG_CONN_MAX_UL).value_or(0);
        p.slotAllocation = c->childInt(EC_TAG_CONN_SLOT_ALLOCATION).value_or(0);
        p.tcpPort = c->childInt(EC_TAG_CONN_TCP_PORT).value_or(0);
        p.udpPort = c->childInt(EC_TAG_CONN_UDP_PORT).value_or(0);
        p.udpDisable = flag(c, EC_TAG_CONN_UDP_DISABLE);
        p.maxSources = c->childInt(EC_TAG_CONN_MAX_FILE_SOURCES).value_or(0);
        p.maxConnections = c->childInt(EC_TAG_CONN_MAX_CONN).value_or(0);
        p.networkEd2k = flag(c, EC_TAG_NETWORK_ED2K);
        p.networkKad = flag(c, EC_TAG_NETWORK_KADEMLIA);
        p.autoconnect = flag(c, EC_TAG_CONN_AUTOCONNECT);
        p.reconnect = flag(c, EC_TAG_CONN_RECONNECT);
    }
    if (const Tag* f = resp->tag(EC_TAG_PREFS_FILES)) {
        p.ichEnabled = flag(f, EC_TAG_FILES_ICH_ENABLED);
        p.aichTrust = flag(f, EC_TAG_FILES_AICH_TRUST);
        p.addNewPaused = flag(f, EC_TAG_FILES_NEW_PAUSED);
        p.addNewAutoDlPrio = flag(f, EC_TAG_FILES_NEW_AUTO_DL_PRIO);
        p.previewPrio = flag(f, EC_TAG_FILES_PREVIEW_PRIO);
        p.addNewAutoUlPrio = flag(f, EC_TAG_FILES_NEW_AUTO_UL_PRIO);
        p.ulFullChunks = flag(f, EC_TAG_FILES_UL_FULL_CHUNKS);
        p.startNextPaused = flag(f, EC_TAG_FILES_START_NEXT_PAUSED);
        p.resumeSameCategory = flag(f, EC_TAG_FILES_RESUME_SAME_CAT);
        p.saveSources = flag(f, EC_TAG_FILES_SAVE_SOURCES);
        p.extractMetadata = flag(f, EC_TAG_FILES_EXTRACT_METADATA);
        p.allocFullSize = flag(f, EC_TAG_FILES_ALLOC_FULL_SIZE);
        p.checkFreeSpace = flag(f, EC_TAG_FILES_CHECK_FREE_SPACE);
        p.minFreeSpaceMB = f->childInt(EC_TAG_FILES_MIN_FREE_SPACE).value_or(0);
    }
    if (const Tag* d = resp->tag(EC_TAG_PREFS_DIRECTORIES)) {
        p.incomingDir = d->childString(EC_TAG_DIRECTORIES_INCOMING).value_or(QString());
        p.tempDir = d->childString(EC_TAG_DIRECTORIES_TEMP).value_or(QString());
        p.shareHidden = intFlag(d, EC_TAG_DIRECTORIES_SHARE_HIDDEN);
        p.autoRescan = intFlag(d, EC_TAG_DIRECTORIES_AUTO_RESCAN);
    }
    if (const Tag* s = resp->tag(EC_TAG_PREFS_SERVERS)) {
        p.removeDeadServers = flag(s, EC_TAG_SERVERS_REMOVE_DEAD);
        p.deadServerRetries = s->childInt(EC_TAG_SERVERS_DEAD_SERVER_RETRIES).value_or(0);
        p.serverAutoUpdate = flag(s, EC_TAG_SERVERS_AUTO_UPDATE);
        p.updateFromServer = flag(s, EC_TAG_SERVERS_ADD_FROM_SERVER);
        p.updateFromClient = flag(s, EC_TAG_SERVERS_ADD_FROM_CLIENT);
        p.useScoreSystem = flag(s, EC_TAG_SERVERS_USE_SCORE_SYSTEM);
        p.smartIdCheck = flag(s, EC_TAG_SERVERS_SMART_ID_CHECK);
        p.safeServerConnect = flag(s, EC_TAG_SERVERS_SAFE_SERVER_CONNECT);
        p.autoConnectStaticOnly = flag(s, EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY);
        p.manualHighPrio = flag(s, EC_TAG_SERVERS_MANUAL_HIGH_PRIO);
        p.serverListUrl = s->childString(EC_TAG_SERVERS_UPDATE_URL).value_or(QString());
    }
    if (const Tag* sec = resp->tag(EC_TAG_PREFS_SECURITY)) {
        p.canSeeShares = sec->childInt(EC_TAG_SECURITY_CAN_SEE_SHARES).value_or(0);
        p.filterClients = flag(sec, EC_TAG_IPFILTER_CLIENTS);
        p.filterServers = flag(sec, EC_TAG_IPFILTER_SERVERS);
        p.ipfilterAutoUpdate = flag(sec, EC_TAG_IPFILTER_AUTO_UPDATE);
        p.ipfilterUrl = sec->childString(EC_TAG_IPFILTER_UPDATE_URL).value_or(QString());
        p.ipfilterLevel = sec->childInt(EC_TAG_IPFILTER_LEVEL).value_or(0);
        p.filterLan = flag(sec, EC_TAG_IPFILTER_FILTER_LAN);
        p.useSecIdent = flag(sec, EC_TAG_SECURITY_USE_SECIDENT);
        p.obfuscationSupported = flag(sec, EC_TAG_SECURITY_OBFUSCATION_SUPPORTED);
        p.obfuscationRequested = flag(sec, EC_TAG_SECURITY_OBFUSCATION_REQUESTED);
        p.obfuscationRequired = flag(sec, EC_TAG_SECURITY_OBFUSCATION_REQUIRED);
    }
    if (const Tag* m = resp->tag(EC_TAG_PREFS_MESSAGEFILTER)) {
        p.msgFilterEnabled = flag(m, EC_TAG_MSGFILTER_ENABLED);
        p.msgFilterAll = flag(m, EC_TAG_MSGFILTER_ALL);
        p.msgFilterFriends = flag(m, EC_TAG_MSGFILTER_FRIENDS);
        p.msgFilterSecure = flag(m, EC_TAG_MSGFILTER_SECURE);
        p.msgFilterByKeyword = flag(m, EC_TAG_MSGFILTER_BY_KEYWORD);
        p.msgKeywords = m->childString(EC_TAG_MSGFILTER_KEYWORDS).value_or(QString());
    }
    if (const Tag* w = resp->tag(EC_TAG_PREFS_REMOTECTRL)) {
        p.webserverAutorun = flag(w, EC_TAG_WEBSERVER_AUTORUN);
        p.webserverPort = w->childInt(EC_TAG_WEBSERVER_PORT).value_or(0);
        p.webserverGuest = flag(w, EC_TAG_WEBSERVER_GUEST);
        p.webserverGzip = flag(w, EC_TAG_WEBSERVER_USEGZIP);
        p.webserverRefresh = w->childInt(EC_TAG_WEBSERVER_REFRESH).value_or(0);
        p.webserverTemplate = w->childString(EC_TAG_WEBSERVER_TEMPLATE).value_or(QString());
    }
    if (const Tag* o = resp->tag(EC_TAG_PREFS_ONLINESIG)) {
        p.onlineSignatureEnabled = flag(o, EC_TAG_ONLINESIG_ENABLED);
    }
    if (const Tag* t = resp->tag(EC_TAG_PREFS_CORETWEAKS)) {
        p.maxConnPer5Sec = t->childInt(EC_TAG_CORETW_MAX_CONN_PER_FIVE).value_or(0);
        p.verbose = flag(t, EC_TAG_CORETW_VERBOSE);
        p.fileBufferSize = t->childInt(EC_TAG_CORETW_FILEBUFFER).value_or(0);
        p.uploadQueueSize = t->childInt(EC_TAG_CORETW_UL_QUEUE).value_or(0);
        p.serverKeepaliveTimeout = t->childInt(EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT).value_or(0);
    }
    if (const Tag* k = resp->tag(EC_TAG_PREFS_KADEMLIA)) {
        p.kadNodesUrl = k->childString(EC_TAG_KADEMLIA_UPDATE_URL).value_or(QString());
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
    // The daemon reads booleans by value, so write every flag as a 0/1 int.
    const auto b = [](bool v) -> quint64 { return v ? 1 : 0; };

    Tag general = Tag::empty(EC_TAG_PREFS_GENERAL);
    general.withChildren({
        Tag::string(EC_TAG_USER_NICK, p.nick),
        Tag::integer(EC_TAG_GENERAL_CHECK_NEW_VERSION, b(p.checkNewVersion)),
    });

    Tag conn = Tag::empty(EC_TAG_PREFS_CONNECTIONS);
    conn.withChildren({
        Tag::integer(EC_TAG_CONN_MAX_DL, p.maxDownload),
        Tag::integer(EC_TAG_CONN_MAX_UL, p.maxUpload),
        Tag::integer(EC_TAG_CONN_SLOT_ALLOCATION, p.slotAllocation),
        Tag::integer(EC_TAG_CONN_TCP_PORT, p.tcpPort),
        Tag::integer(EC_TAG_CONN_UDP_PORT, p.udpPort),
        Tag::integer(EC_TAG_CONN_UDP_DISABLE, b(p.udpDisable)),
        Tag::integer(EC_TAG_CONN_MAX_FILE_SOURCES, p.maxSources),
        Tag::integer(EC_TAG_CONN_MAX_CONN, p.maxConnections),
        Tag::integer(EC_TAG_CONN_AUTOCONNECT, b(p.autoconnect)),
        Tag::integer(EC_TAG_CONN_RECONNECT, b(p.reconnect)),
        Tag::integer(EC_TAG_NETWORK_ED2K, b(p.networkEd2k)),
        Tag::integer(EC_TAG_NETWORK_KADEMLIA, b(p.networkKad)),
    });

    Tag files = Tag::empty(EC_TAG_PREFS_FILES);
    files.withChildren({
        Tag::integer(EC_TAG_FILES_ICH_ENABLED, b(p.ichEnabled)),
        Tag::integer(EC_TAG_FILES_AICH_TRUST, b(p.aichTrust)),
        Tag::integer(EC_TAG_FILES_NEW_PAUSED, b(p.addNewPaused)),
        Tag::integer(EC_TAG_FILES_NEW_AUTO_DL_PRIO, b(p.addNewAutoDlPrio)),
        Tag::integer(EC_TAG_FILES_PREVIEW_PRIO, b(p.previewPrio)),
        Tag::integer(EC_TAG_FILES_NEW_AUTO_UL_PRIO, b(p.addNewAutoUlPrio)),
        Tag::integer(EC_TAG_FILES_UL_FULL_CHUNKS, b(p.ulFullChunks)),
        Tag::integer(EC_TAG_FILES_START_NEXT_PAUSED, b(p.startNextPaused)),
        Tag::integer(EC_TAG_FILES_RESUME_SAME_CAT, b(p.resumeSameCategory)),
        Tag::integer(EC_TAG_FILES_SAVE_SOURCES, b(p.saveSources)),
        Tag::integer(EC_TAG_FILES_EXTRACT_METADATA, b(p.extractMetadata)),
        Tag::integer(EC_TAG_FILES_ALLOC_FULL_SIZE, b(p.allocFullSize)),
        Tag::integer(EC_TAG_FILES_CHECK_FREE_SPACE, b(p.checkFreeSpace)),
        Tag::integer(EC_TAG_FILES_MIN_FREE_SPACE, p.minFreeSpaceMB),
    });

    Tag dir = Tag::empty(EC_TAG_PREFS_DIRECTORIES);
    dir.withChildren({
        Tag::string(EC_TAG_DIRECTORIES_INCOMING, p.incomingDir),
        Tag::string(EC_TAG_DIRECTORIES_TEMP, p.tempDir),
        Tag::integer(EC_TAG_DIRECTORIES_SHARE_HIDDEN, b(p.shareHidden)),
        Tag::integer(EC_TAG_DIRECTORIES_AUTO_RESCAN, b(p.autoRescan)),
    });

    Tag servers = Tag::empty(EC_TAG_PREFS_SERVERS);
    servers.withChildren({
        Tag::integer(EC_TAG_SERVERS_REMOVE_DEAD, b(p.removeDeadServers)),
        Tag::integer(EC_TAG_SERVERS_DEAD_SERVER_RETRIES, p.deadServerRetries),
        Tag::integer(EC_TAG_SERVERS_AUTO_UPDATE, b(p.serverAutoUpdate)),
        Tag::integer(EC_TAG_SERVERS_ADD_FROM_SERVER, b(p.updateFromServer)),
        Tag::integer(EC_TAG_SERVERS_ADD_FROM_CLIENT, b(p.updateFromClient)),
        Tag::integer(EC_TAG_SERVERS_USE_SCORE_SYSTEM, b(p.useScoreSystem)),
        Tag::integer(EC_TAG_SERVERS_SMART_ID_CHECK, b(p.smartIdCheck)),
        Tag::integer(EC_TAG_SERVERS_SAFE_SERVER_CONNECT, b(p.safeServerConnect)),
        Tag::integer(EC_TAG_SERVERS_AUTOCONN_STATIC_ONLY, b(p.autoConnectStaticOnly)),
        Tag::integer(EC_TAG_SERVERS_MANUAL_HIGH_PRIO, b(p.manualHighPrio)),
    });

    Tag security = Tag::empty(EC_TAG_PREFS_SECURITY);
    security.withChildren({
        Tag::integer(EC_TAG_SECURITY_CAN_SEE_SHARES, p.canSeeShares),
        Tag::integer(EC_TAG_IPFILTER_CLIENTS, b(p.filterClients)),
        Tag::integer(EC_TAG_IPFILTER_SERVERS, b(p.filterServers)),
        Tag::integer(EC_TAG_IPFILTER_AUTO_UPDATE, b(p.ipfilterAutoUpdate)),
        Tag::string(EC_TAG_IPFILTER_UPDATE_URL, p.ipfilterUrl),
        Tag::integer(EC_TAG_IPFILTER_LEVEL, p.ipfilterLevel),
        Tag::integer(EC_TAG_IPFILTER_FILTER_LAN, b(p.filterLan)),
        Tag::integer(EC_TAG_SECURITY_USE_SECIDENT, b(p.useSecIdent)),
        Tag::integer(EC_TAG_SECURITY_OBFUSCATION_REQUESTED, b(p.obfuscationRequested)),
        Tag::integer(EC_TAG_SECURITY_OBFUSCATION_REQUIRED, b(p.obfuscationRequired)),
    });

    Tag msg = Tag::empty(EC_TAG_PREFS_MESSAGEFILTER);
    msg.withChildren({
        Tag::integer(EC_TAG_MSGFILTER_ENABLED, b(p.msgFilterEnabled)),
        Tag::integer(EC_TAG_MSGFILTER_ALL, b(p.msgFilterAll)),
        Tag::integer(EC_TAG_MSGFILTER_FRIENDS, b(p.msgFilterFriends)),
        Tag::integer(EC_TAG_MSGFILTER_SECURE, b(p.msgFilterSecure)),
        Tag::integer(EC_TAG_MSGFILTER_BY_KEYWORD, b(p.msgFilterByKeyword)),
        Tag::string(EC_TAG_MSGFILTER_KEYWORDS, p.msgKeywords),
    });

    Tag web = Tag::empty(EC_TAG_PREFS_REMOTECTRL);
    web.withChildren({
        Tag::integer(EC_TAG_WEBSERVER_AUTORUN, b(p.webserverAutorun)),
        Tag::integer(EC_TAG_WEBSERVER_PORT, p.webserverPort),
        Tag::integer(EC_TAG_WEBSERVER_GUEST, b(p.webserverGuest)),
        Tag::integer(EC_TAG_WEBSERVER_USEGZIP, b(p.webserverGzip)),
        Tag::integer(EC_TAG_WEBSERVER_REFRESH, p.webserverRefresh),
        Tag::string(EC_TAG_WEBSERVER_TEMPLATE, p.webserverTemplate),
    });

    Tag onlinesig = Tag::empty(EC_TAG_PREFS_ONLINESIG);
    onlinesig.withChildren({
        Tag::integer(EC_TAG_ONLINESIG_ENABLED, b(p.onlineSignatureEnabled)),
    });

    Tag tweaks = Tag::empty(EC_TAG_PREFS_CORETWEAKS);
    tweaks.withChildren({
        Tag::integer(EC_TAG_CORETW_MAX_CONN_PER_FIVE, p.maxConnPer5Sec),
        Tag::integer(EC_TAG_CORETW_VERBOSE, b(p.verbose)),
        Tag::integer(EC_TAG_CORETW_FILEBUFFER, p.fileBufferSize),
        Tag::integer(EC_TAG_CORETW_UL_QUEUE, p.uploadQueueSize),
        Tag::integer(EC_TAG_CORETW_SRV_KEEPALIVE_TIMEOUT, p.serverKeepaliveTimeout),
    });

    Tag kad = Tag::empty(EC_TAG_PREFS_KADEMLIA);
    kad.withChildren({
        Tag::string(EC_TAG_KADEMLIA_UPDATE_URL, p.kadNodesUrl),
    });

    return sendDiscard(client, Packet(EC_OP_SET_PREFERENCES,
                                      {general, conn, files, dir, servers, security,
                                       msg, web, onlinesig, tweaks, kad}));
}

} // namespace amule
