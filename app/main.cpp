#include <QApplication>
#include <QIcon>
#include <QSettings>
#include <QTimer>

#include "ec/worker.h"
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("amule-remote-qt"));
    QApplication::setApplicationName(QStringLiteral("amule-remote-qt"));
    QApplication::setApplicationDisplayName(QStringLiteral("aMule Remote"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));

    // ADR-0006: settings persisted as INI under the platform config location.
    QSettings::setDefaultFormat(QSettings::IniFormat);

    amule::registerEcMetaTypes();

    amule::MainWindow window;
    window.resize(960, 600);
    window.show();

    // Headless launch check: `--selftest` quits shortly after starting so the
    // window can be exercised under the offscreen platform without a display.
    if (app.arguments().contains(QStringLiteral("--selftest")))
        QTimer::singleShot(2000, &app, &QCoreApplication::quit);

    return app.exec();
}
