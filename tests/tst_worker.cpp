// Tests for the EcWorker / EcWorkerThread (ADR-0004). The offline tests are
// deterministic and exercise the threading, queued command dispatch and signal
// emission without a daemon. A live test drives the full worker against a real
// amuled and skips when no credentials are configured.

#include <memory>

#include <QObject>
#include <QSignalSpy>
#include <QtTest>

#include "ec/worker.h"
#include "livecreds.h"

using namespace amule;

class TestWorker : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { registerEcMetaTypes(); }

    void commandIgnoredWhenDisconnected();
    void connectionRefusedEmitsError();
    void liveWorkerRefreshesState();
};

void TestWorker::commandIgnoredWhenDisconnected() {
    EcWorkerThread thread;
    QSignalSpy logSpy(thread.worker(), &EcWorker::logMessage);
    QVERIFY(logSpy.isValid());

    // A command needing a connection must be ignored with a log message.
    QMetaObject::invokeMethod(thread.worker(), "pause", Qt::QueuedConnection,
                              Q_ARG(amule::Hash16, Hash16{}));

    QVERIFY(logSpy.wait(2000));
    QVERIFY(logSpy.constFirst().constFirst().toString().contains(
        QStringLiteral("not connected")));
}

void TestWorker::connectionRefusedEmitsError() {
    EcWorkerThread thread;
    QSignalSpy statusSpy(thread.worker(), &EcWorker::statusChanged);
    QVERIFY(statusSpy.isValid());

    // Port 1 on localhost refuses quickly: expect Connecting then Error.
    QMetaObject::invokeMethod(thread.worker(), "connectToDaemon", Qt::QueuedConnection,
                              Q_ARG(QString, QStringLiteral("127.0.0.1")),
                              Q_ARG(quint16, quint16{1}),
                              Q_ARG(QString, QStringLiteral("irrelevant")));

    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.size() >= 2, 5000);
    QCOMPARE(statusSpy.constFirst().constFirst().value<ConnStatus>(),
             ConnStatus::Connecting);
    QCOMPARE(statusSpy.constLast().constFirst().value<ConnStatus>(),
             ConnStatus::Error);
}

void TestWorker::liveWorkerRefreshesState() {
    const test::Credentials creds = test::loadCredentials();
    if (!creds.ok)
        QSKIP("no amuled credentials in env or ~/.config/rstamule/config.ini");

    EcWorkerThread thread;
    QSignalSpy statusSpy(thread.worker(), &EcWorker::statusChanged);
    QSignalSpy statsSpy(thread.worker(), &EcWorker::statsUpdated);
    QSignalSpy downloadsSpy(thread.worker(), &EcWorker::downloadsUpdated);

    const QString password =
        creds.passwordMd5.isEmpty() ? creds.passwordPlain : creds.passwordMd5;
    QMetaObject::invokeMethod(thread.worker(), "connectToDaemon", Qt::QueuedConnection,
                              Q_ARG(QString, creds.host), Q_ARG(quint16, creds.port),
                              Q_ARG(QString, password));

    // Wait for a terminal status; skip if the daemon is unreachable.
    QTRY_VERIFY_WITH_TIMEOUT(!statusSpy.isEmpty(), 8000);
    const auto lastStatus = [&] {
        return statusSpy.constLast().constFirst().value<ConnStatus>();
    };
    QTRY_VERIFY_WITH_TIMEOUT(lastStatus() == ConnStatus::Connected ||
                                 lastStatus() == ConnStatus::Error,
                             8000);
    if (lastStatus() == ConnStatus::Error)
        QSKIP("could not reach amuled — skipping live worker test");

    // The worker should publish a stats snapshot and a (possibly empty) queue.
    QTRY_VERIFY_WITH_TIMEOUT(!statsSpy.isEmpty(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(!downloadsSpy.isEmpty(), 5000);
    qInfo() << "worker received" << statsSpy.size() << "stats updates and"
            << downloadsSpy.size() << "queue updates";
}

QTEST_MAIN(TestWorker)
#include "tst_worker.moc"
