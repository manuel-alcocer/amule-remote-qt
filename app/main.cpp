#include <QApplication>
#include <QFont>
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

    // Nudge the default font up 2 points — bundled/AppImage runs often default to
    // a small base font without the desktop's font settings.
    {
        QFont font = QApplication::font();
        if (font.pointSizeF() > 0)
            font.setPointSizeF(font.pointSizeF() + 2);
        else
            font.setPixelSize(font.pixelSize() + 3);
        QApplication::setFont(font);
    }

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
