#include "ec/worker.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <QMetaObject>
#include <QTimer>

namespace amule {
namespace {

constexpr int kRefreshIntervalMs = 1000;
constexpr qint64 kSearchGraceMs = 12'000;

template <typename T>
QList<T> toQList(const std::vector<T>& v) {
    QList<T> out;
    out.reserve(static_cast<qsizetype>(v.size()));
    for (const T& item : v)
        out.append(item);
    return out;
}

} // namespace

EcWorker::EcWorker(QObject* parent) : QObject(parent) {
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(kRefreshIntervalMs);
    connect(refreshTimer_, &QTimer::timeout, this, &EcWorker::onRefresh);
}

EcWorker::~EcWorker() = default;

void EcWorker::connectToDaemon(const QString& host, quint16 port,
                               const QString& password) {
    sourceClients_.clear();
    emit statusChanged(ConnStatus::Connecting,
                       QStringLiteral("%1:%2").arg(host).arg(port));
    emit logMessage(QStringLiteral("connecting to %1:%2…").arg(host).arg(port));

    // Accept either a plaintext password or an already-hashed 32-char MD5 hex
    // digest (the form aMule stores); normalize to the digest either way.
    const QString md5 = ec::normalizePasswordMd5(password);
    auto exp = ec::EcClient::connectWithMd5(host, port, md5);
    if (!exp) {
        emit logMessage(QStringLiteral("connection failed: %1").arg(exp.error().message));
        emit statusChanged(ConnStatus::Error, exp.error().message);
        return;
    }

    client_ = std::move(*exp);
    emit logMessage(QStringLiteral("connected — daemon %1").arg(client_->serverVersion()));
    emit statusChanged(ConnStatus::Connected, client_->serverVersion());

    // Load daemon preferences once on connect.
    if (auto prefs = amule::fetchPrefs(*client_))
        emit prefsUpdated(*prefs);

    refreshTimer_->start();
    onRefresh(); // refresh immediately
}

void EcWorker::disconnectFromDaemon() {
    teardown(QStringLiteral("disconnected"), ConnStatus::Disconnected);
    emit logMessage(QStringLiteral("disconnected"));
}

void EcWorker::shutdown() {
    if (refreshTimer_)
        refreshTimer_->stop();
    client_.reset();
    sourceClients_.clear();
}

void EcWorker::teardown(const QString& reason, ConnStatus status) {
    if (refreshTimer_)
        refreshTimer_->stop();
    client_.reset();
    sourceClients_.clear();
    if (searching_) {
        searching_ = false;
        emit searchingChanged(false);
    }
    emit statusChanged(status, reason);
}

template <typename Fn>
void EcWorker::runConnected(Fn&& fn) {
    if (!client_) {
        emit logMessage(QStringLiteral("not connected — command ignored"));
        return;
    }
    auto result = std::forward<Fn>(fn)(*client_);
    if (!result) {
        const ec::EcError& e = result.error();
        emit logMessage(QStringLiteral("command failed: %1").arg(e.message));
        // Only a transport error invalidates the connection; protocol-level
        // refusals (e.g. "search failed") leave it usable.
        if (e.kind == ec::EcError::Kind::Io)
            teardown(e.message, ConnStatus::Error);
        return;
    }
    onRefresh(); // immediate refresh for snappy feedback
}

// --- Download commands -----------------------------------------------------

void EcWorker::pause(Hash16 hash) {
    runConnected([&](ec::EcClient& c) { return amule::pauseDownload(c, hash); });
}
void EcWorker::resume(Hash16 hash) {
    runConnected([&](ec::EcClient& c) { return amule::resumeDownload(c, hash); });
}
void EcWorker::stop(Hash16 hash) {
    runConnected([&](ec::EcClient& c) { return amule::stopDownload(c, hash); });
}
void EcWorker::remove(Hash16 hash) {
    runConnected([&](ec::EcClient& c) { return amule::deleteDownload(c, hash); });
}
void EcWorker::setPriority(Hash16 hash, quint8 prio) {
    runConnected([&](ec::EcClient& c) { return amule::setPriority(c, hash, prio); });
}
void EcWorker::clearCompleted() {
    runConnected([](ec::EcClient& c) { return amule::clearCompleted(c); });
}
void EcWorker::addLink(const QString& link) {
    runConnected([&](ec::EcClient& c) { return amule::addEd2kLink(c, link); });
}

// --- Networks --------------------------------------------------------------

void EcWorker::connectNetworks() {
    runConnected([](ec::EcClient& c) { return amule::connectNetworks(c); });
}
void EcWorker::disconnectNetworks() {
    runConnected([](ec::EcClient& c) { return amule::disconnectNetworks(c); });
}
void EcWorker::startKad() {
    runConnected([](ec::EcClient& c) { return amule::startKad(c); });
}
void EcWorker::stopKad() {
    runConnected([](ec::EcClient& c) { return amule::stopKad(c); });
}

// --- Search ----------------------------------------------------------------

void EcWorker::startSearch(SearchParams params) {
    if (!client_) {
        emit logMessage(QStringLiteral("not connected — command ignored"));
        return;
    }
    searching_ = true;
    at100_ = false;
    emit searchingChanged(true);
    emit searchResultsUpdated({});
    emit searchProgressChanged(0);
    emit logMessage(QStringLiteral("searching for \"%1\"…").arg(params.text));

    auto result = amule::startSearch(*client_, params);
    if (!result) {
        searching_ = false;
        emit searchingChanged(false);
        emit logMessage(QStringLiteral("search failed: %1").arg(result.error().message));
        if (result.error().kind == ec::EcError::Kind::Io)
            teardown(result.error().message, ConnStatus::Error);
        return;
    }
    onRefresh();
}

void EcWorker::stopSearch() {
    if (searching_) {
        searching_ = false;
        emit searchingChanged(false);
    }
    runConnected([](ec::EcClient& c) { return amule::stopSearch(c); });
}

void EcWorker::downloadResult(Hash16 hash) {
    runConnected([&](ec::EcClient& c) { return amule::downloadSearchResult(c, hash); });
}

// --- eD2k servers ----------------------------------------------------------

void EcWorker::serverConnect(ServerIp ip, quint16 port) {
    runConnected([&](ec::EcClient& c) { return amule::serverConnect(c, ip, port); });
}
void EcWorker::serverConnectAny() {
    runConnected([](ec::EcClient& c) { return amule::serverConnectAny(c); });
}
void EcWorker::serverDisconnect() {
    runConnected([](ec::EcClient& c) { return amule::serverDisconnect(c); });
}
void EcWorker::serverRemove(ServerIp ip, quint16 port) {
    runConnected([&](ec::EcClient& c) { return amule::serverRemove(c, ip, port); });
}
void EcWorker::serverAdd(const QString& name, const QString& address) {
    runConnected([&](ec::EcClient& c) { return amule::serverAdd(c, name, address); });
}
void EcWorker::serverUpdateFromUrl(const QString& url) {
    runConnected([&](ec::EcClient& c) { return amule::serverUpdateFromUrl(c, url); });
}

// --- Shared files & preferences --------------------------------------------

void EcWorker::reloadShared() {
    runConnected([](ec::EcClient& c) { return amule::reloadSharedFiles(c); });
}

void EcWorker::fetchPrefs() {
    if (!client_) {
        emit logMessage(QStringLiteral("not connected — command ignored"));
        return;
    }
    auto prefs = amule::fetchPrefs(*client_);
    if (!prefs) {
        emit logMessage(QStringLiteral("fetch prefs failed: %1").arg(prefs.error().message));
        if (prefs.error().kind == ec::EcError::Kind::Io)
            teardown(prefs.error().message, ConnStatus::Error);
        return;
    }
    emit prefsUpdated(*prefs);
}

void EcWorker::applyPrefs(DaemonPrefs prefs) {
    if (!client_) {
        emit logMessage(QStringLiteral("not connected — command ignored"));
        return;
    }
    auto result = amule::setPrefs(*client_, prefs);
    if (!result) {
        emit logMessage(QStringLiteral("apply prefs failed: %1").arg(result.error().message));
        if (result.error().kind == ec::EcError::Kind::Io)
            teardown(result.error().message, ConnStatus::Error);
        return;
    }
    emit logMessage(QStringLiteral("preferences applied"));
    if (auto updated = amule::fetchPrefs(*client_))
        emit prefsUpdated(*updated);
}

// --- Periodic refresh ------------------------------------------------------

void EcWorker::onRefresh() {
    if (!client_)
        return;

    const auto fail = [&](const ec::EcError& e) {
        emit logMessage(QStringLiteral("connection lost: %1").arg(e.message));
        teardown(e.message, ConnStatus::Error);
    };

    auto stats = amule::fetchStats(*client_);
    if (!stats)
        return fail(stats.error());
    auto downloads = amule::fetchDownloads(*client_);
    if (!downloads)
        return fail(downloads.error());
    auto conn = amule::fetchConnState(*client_);
    if (!conn)
        return fail(conn.error());
    auto servers = amule::fetchServers(*client_);
    if (!servers)
        return fail(servers.error());
    auto sharedFiles = amule::fetchSharedFiles(*client_);
    if (!sharedFiles)
        return fail(sharedFiles.error());

    // Per-source detail (incremental), most active sources first.
    if (auto upd = amule::fetchClientUpdate(*client_, sourceClients_); !upd)
        return fail(upd.error());
    std::vector<SourceClient> sources;
    sources.reserve(sourceClients_.size());
    for (const auto& [id, client] : sourceClients_)
        sources.push_back(client);
    std::stable_sort(sources.begin(), sources.end(),
                     [](const SourceClient& a, const SourceClient& b) {
                         return a.downSpeed > b.downSpeed;
                     });

    emit statsUpdated(*stats);
    emit downloadsUpdated(toQList(*downloads));
    emit connStateUpdated(*conn);
    emit serversUpdated(toQList(*servers));
    emit sharedFilesUpdated(toQList(*sharedFiles));
    emit sourcesUpdated(toQList(sources));

    if (searching_) {
        auto progress = amule::fetchSearchProgress(*client_);
        if (!progress)
            return fail(progress.error());
        auto results = amule::fetchSearchResults(*client_);
        if (!results)
            return fail(results.error());
        emit searchProgressChanged(*progress);
        emit searchResultsUpdated(toQList(*results));

        // Keep gathering results for a grace period after reaching 100% (global
        // ed2k results keep trickling in past that point).
        if (*progress >= 100) {
            if (!at100_) {
                at100_ = true;
                at100Timer_.start();
            } else if (at100Timer_.elapsed() >= kSearchGraceMs) {
                searching_ = false;
                at100_ = false;
                emit searchingChanged(false);
            }
        } else {
            at100_ = false;
        }
    }
}

// --- Metatypes & thread owner ----------------------------------------------

void registerEcMetaTypes() {
    qRegisterMetaType<ConnStatus>();
    qRegisterMetaType<Hash16>();
    qRegisterMetaType<ServerIp>();
    qRegisterMetaType<Stats>();
    qRegisterMetaType<ConnState>();
    qRegisterMetaType<DaemonPrefs>();
    qRegisterMetaType<SearchParams>();
    qRegisterMetaType<Download>();
    qRegisterMetaType<Server>();
    qRegisterMetaType<SharedFile>();
    qRegisterMetaType<SourceClient>();
    qRegisterMetaType<SearchResult>();
    qRegisterMetaType<QList<Download>>();
    qRegisterMetaType<QList<Server>>();
    qRegisterMetaType<QList<SharedFile>>();
    qRegisterMetaType<QList<SourceClient>>();
    qRegisterMetaType<QList<SearchResult>>();
}

EcWorkerThread::EcWorkerThread() {
    registerEcMetaTypes();
    worker_ = new EcWorker();
    worker_->moveToThread(&thread_);
    QObject::connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    thread_.start();
}

EcWorkerThread::~EcWorkerThread() {
    // Release the socket on the worker thread before stopping it.
    QMetaObject::invokeMethod(worker_, &EcWorker::shutdown, Qt::BlockingQueuedConnection);
    thread_.quit();
    thread_.wait();
}

} // namespace amule
