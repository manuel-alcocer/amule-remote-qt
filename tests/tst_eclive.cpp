// Live smoke test against a real amuled. Credentials are read ONLY from the
// environment or from the external ~/.config/rstamule/config.ini — never from
// this repository. The test skips gracefully when no credentials are configured
// or the daemon is unreachable, so it is safe to run anywhere.
//
//   AMULE_HOST, AMULE_PORT, AMULE_PASSWORD_MD5 (or AMULE_PASSWORD) override the
//   config file. Run with:  ctest -R eclive --output-on-failure

#include <memory>

#include <QObject>
#include <QString>
#include <QtTest>

#include "ec/client.h"
#include "ec/queries.h"
#include "livecreds.h"

using namespace amule;
using test::Credentials;
using test::loadCredentials;

class TestEcLive : public QObject {
    Q_OBJECT

private slots:
    void connectAuthenticateAndQuery();
};

void TestEcLive::connectAuthenticateAndQuery() {
    const Credentials creds = loadCredentials();
    if (!creds.ok)
        QSKIP("no amuled credentials in env or ~/.config/rstamule/config.ini");

    auto clientExp =
        creds.passwordMd5.isEmpty()
            ? ec::EcClient::connect(creds.host, creds.port, creds.passwordPlain)
            : ec::EcClient::connectWithMd5(creds.host, creds.port, creds.passwordMd5);

    if (!clientExp) {
        // Daemon down / unreachable / wrong creds: skip rather than fail so the
        // suite stays green on machines without a live daemon.
        QSKIP(qPrintable(QStringLiteral("could not reach amuled at %1:%2 — %3")
                             .arg(creds.host)
                             .arg(creds.port)
                             .arg(clientExp.error().message)));
    }

    std::unique_ptr<ec::EcClient> client = std::move(*clientExp);
    qInfo().noquote() << "Authenticated against amuled, server version:"
                      << client->serverVersion();

    // Statistics.
    auto stats = fetchStats(*client);
    QVERIFY2(stats.has_value(), qPrintable(stats ? QString() : stats.error().message));
    qInfo().noquote() << "  down:" << humanRate(stats->dlSpeed)
                      << " up:" << humanRate(stats->ulSpeed)
                      << " ed2k users:" << humanCount(stats->ed2kUsers)
                      << " high id:" << stats->isHighId();

    // Connection state.
    auto conn = fetchConnState(*client);
    QVERIFY2(conn.has_value(), qPrintable(conn ? QString() : conn.error().message));
    qInfo().noquote() << "  ed2k connected:" << conn->ed2kConnected
                      << " server:" << conn->ed2kServer
                      << " kad:" << conn->kadConnected;

    // Download queue.
    auto downloads = fetchDownloads(*client);
    QVERIFY2(downloads.has_value(),
             qPrintable(downloads ? QString() : downloads.error().message));
    qInfo().noquote() << "  downloads in queue:" << downloads->size();
    for (const Download& d : *downloads) {
        qInfo().noquote() << "   -" << d.name << '['
                          << qPrintable(d.statusLabel()) << ']'
                          << humanSizePair(d.sizeDone, d.sizeFull)
                          << humanRate(d.speed);
    }

    // Shared files (count only, to keep output small).
    auto shared = fetchSharedFiles(*client);
    QVERIFY2(shared.has_value(),
             qPrintable(shared ? QString() : shared.error().message));
    qInfo().noquote() << "  shared files:" << shared->size();
}

QTEST_MAIN(TestEcLive)
#include "tst_eclive.moc"
