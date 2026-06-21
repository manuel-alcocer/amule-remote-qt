// Background worker that owns the EC connection (ADR-0004).
//
// The UI never touches the socket: it invokes the worker's command slots (queued
// across the thread boundary) and receives state via signals carrying copied
// value objects — no shared mutex. A QTimer drives the ~1 s refresh while
// connected. Ported from rstamule-cli's worker.rs, expressed with Qt threading.

#pragma once

#include <array>
#include <memory>
#include <unordered_map>

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QString>
#include <QThread>
#include <QtTypes>

#include "ec/client.h"
#include "ec/queries.h"
#include "model/model.h"

class QTimer;

namespace amule {

// Connection lifecycle as observed by the UI.
enum class ConnStatus { Disconnected, Connecting, Connected, Error };

// Lives on its own thread; owns the EcClient. All slots run on the worker
// thread. Commands that need a connection are ignored (with a log message) when
// disconnected.
class EcWorker : public QObject {
    Q_OBJECT

public:
    explicit EcWorker(QObject* parent = nullptr);
    ~EcWorker() override;

public slots:
    void connectToDaemon(const QString& host, quint16 port, const QString& password);
    void disconnectFromDaemon();

    void pause(amule::Hash16 hash);
    void resume(amule::Hash16 hash);
    void stop(amule::Hash16 hash);
    void remove(amule::Hash16 hash);
    void setPriority(amule::Hash16 hash, quint8 prio);
    void clearCompleted();
    void addLink(const QString& link);

    void connectNetworks();
    void disconnectNetworks();
    void startKad();
    void stopKad();

    void startSearch(amule::SearchParams params);
    void stopSearch();
    void downloadResult(amule::Hash16 hash);

    void serverConnect(amule::ServerIp ip, quint16 port);
    void serverConnectAny();
    void serverDisconnect();
    void serverRemove(amule::ServerIp ip, quint16 port);
    void serverAdd(const QString& name, const QString& address);
    void serverUpdateFromUrl(const QString& url);

    void reloadShared();
    void fetchPrefs();
    void applyPrefs(amule::DaemonPrefs prefs);

    // Stop the timer and release the socket on the worker thread. Invoke with a
    // blocking queued connection before tearing the thread down.
    void shutdown();

signals:
    void statusChanged(amule::ConnStatus status, const QString& detail);
    void logMessage(const QString& message);
    void statsUpdated(amule::Stats stats);
    void downloadsUpdated(QList<amule::Download> downloads);
    void connStateUpdated(amule::ConnState conn);
    void serversUpdated(QList<amule::Server> servers);
    void sharedFilesUpdated(QList<amule::SharedFile> files);
    void sourcesUpdated(QList<amule::SourceClient> sources);
    void searchProgressChanged(quint32 progress);
    void searchResultsUpdated(QList<amule::SearchResult> results);
    void searchingChanged(bool searching);
    void prefsUpdated(amule::DaemonPrefs prefs);

private slots:
    void onRefresh();

private:
    // Run a command that requires a live connection. On an I/O error the
    // connection is torn down; other (remote/protocol) errors are only logged.
    template <typename Fn>
    void runConnected(Fn&& fn);

    void teardown(const QString& reason, ConnStatus status);

    std::unique_ptr<ec::EcClient> client_;
    std::unordered_map<quint32, SourceClient> sourceClients_;
    QTimer* refreshTimer_ = nullptr;
    bool searching_ = false;
    bool at100_ = false;
    QElapsedTimer at100Timer_;
};

// Register the model/worker value types as Qt metatypes so they can cross the
// thread boundary through queued signals/slots. Idempotent; call once.
void registerEcMetaTypes();

// RAII owner of the worker and its thread. Construct on the UI thread; connect
// your UI to `worker()`'s slots/signals. Destruction stops the thread cleanly.
class EcWorkerThread {
public:
    EcWorkerThread();
    ~EcWorkerThread();

    EcWorkerThread(const EcWorkerThread&) = delete;
    EcWorkerThread& operator=(const EcWorkerThread&) = delete;

    [[nodiscard]] EcWorker* worker() const { return worker_; }

private:
    QThread thread_;
    EcWorker* worker_ = nullptr;
};

} // namespace amule

Q_DECLARE_METATYPE(amule::ConnStatus)
Q_DECLARE_METATYPE(amule::Hash16)
Q_DECLARE_METATYPE(amule::ServerIp)
Q_DECLARE_METATYPE(amule::Stats)
Q_DECLARE_METATYPE(amule::Download)
Q_DECLARE_METATYPE(amule::ConnState)
Q_DECLARE_METATYPE(amule::Server)
Q_DECLARE_METATYPE(amule::SharedFile)
Q_DECLARE_METATYPE(amule::SourceClient)
Q_DECLARE_METATYPE(amule::SearchResult)
Q_DECLARE_METATYPE(amule::SearchParams)
Q_DECLARE_METATYPE(amule::DaemonPrefs)
