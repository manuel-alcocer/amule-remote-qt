// High-level EC queries and commands built on top of EcClient. These translate
// EC packets to and from the domain model. Ported from rstamule-cli's
// queries.rs.

#pragma once

#include <expected>
#include <unordered_map>
#include <vector>

#include <QtTypes>

#include "ec/client.h"
#include "model/model.h"

namespace amule {

// --- Reads -----------------------------------------------------------------

std::expected<Stats, ec::EcError> fetchStats(ec::EcClient& client);
std::expected<std::vector<Download>, ec::EcError>
fetchDownloads(ec::EcClient& client);
std::expected<ConnState, ec::EcError> fetchConnState(ec::EcClient& client);
std::expected<std::vector<Server>, ec::EcError>
fetchServers(ec::EcClient& client);
std::expected<std::vector<SharedFile>, ec::EcError>
fetchSharedFiles(ec::EcClient& client);
std::expected<DaemonPrefs, ec::EcError> fetchPrefs(ec::EcClient& client);
std::expected<std::vector<SearchResult>, ec::EcError>
fetchSearchResults(ec::EcClient& client);
std::expected<quint32, ec::EcError> fetchSearchProgress(ec::EcClient& client);

// Fetch the incremental client/source update and merge it into `clients`.
std::expected<void, ec::EcError>
fetchClientUpdate(ec::EcClient& client,
                  std::unordered_map<quint32, SourceClient>& clients);

// --- Download commands -----------------------------------------------------

std::expected<void, ec::EcError> pauseDownload(ec::EcClient& client, Hash16 hash);
std::expected<void, ec::EcError> resumeDownload(ec::EcClient& client, Hash16 hash);
std::expected<void, ec::EcError> stopDownload(ec::EcClient& client, Hash16 hash);
std::expected<void, ec::EcError> deleteDownload(ec::EcClient& client, Hash16 hash);
std::expected<void, ec::EcError>
setPriority(ec::EcClient& client, Hash16 hash, quint8 prio);
std::expected<void, ec::EcError> clearCompleted(ec::EcClient& client);
std::expected<void, ec::EcError>
addEd2kLink(ec::EcClient& client, const QString& link);

// --- Search ----------------------------------------------------------------

std::expected<void, ec::EcError>
startSearch(ec::EcClient& client, const SearchParams& params);
std::expected<void, ec::EcError> stopSearch(ec::EcClient& client);
std::expected<void, ec::EcError>
downloadSearchResult(ec::EcClient& client, Hash16 hash);

// --- Networks --------------------------------------------------------------

std::expected<void, ec::EcError> connectNetworks(ec::EcClient& client);
std::expected<void, ec::EcError> disconnectNetworks(ec::EcClient& client);
std::expected<void, ec::EcError> startKad(ec::EcClient& client);
std::expected<void, ec::EcError> stopKad(ec::EcClient& client);

// --- eD2k servers ----------------------------------------------------------

std::expected<void, ec::EcError>
serverConnect(ec::EcClient& client, std::array<quint8, 4> ip, quint16 port);
std::expected<void, ec::EcError> serverConnectAny(ec::EcClient& client);
std::expected<void, ec::EcError> serverDisconnect(ec::EcClient& client);
std::expected<void, ec::EcError>
serverRemove(ec::EcClient& client, std::array<quint8, 4> ip, quint16 port);
std::expected<void, ec::EcError>
serverAdd(ec::EcClient& client, const QString& name, const QString& address);
std::expected<void, ec::EcError>
serverUpdateFromUrl(ec::EcClient& client, const QString& url);

// --- Shared files & preferences --------------------------------------------

std::expected<void, ec::EcError> reloadSharedFiles(ec::EcClient& client);
std::expected<void, ec::EcError>
setPrefs(ec::EcClient& client, const DaemonPrefs& prefs);

} // namespace amule
