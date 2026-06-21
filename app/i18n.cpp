#include "i18n.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

namespace amule::i18n {

QList<Language> languages() {
    return {
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("es"), QStringLiteral("Español")},
    };
}

QString currentSetting() {
    return QSettings().value(QStringLiteral("language")).toString();
}

void setSetting(const QString& code) {
    QSettings().setValue(QStringLiteral("language"), code);
}

void install(QApplication& app) {
    QString code = currentSetting();
    if (code.isEmpty())
        code = QLocale::system().name().left(2); // "es_ES" -> "es"

    // English is the source language: nothing to load.
    if (code == QLatin1String("en"))
        return;

    // Application strings (bundled in i18n.qrc).
    static QTranslator appTranslator;
    if (appTranslator.load(QStringLiteral(":/i18n/amule-remote-qt_%1").arg(code)))
        app.installTranslator(&appTranslator);

    // Qt's own strings (standard dialog buttons, etc.) — best effort.
    static QTranslator qtTranslator;
    const QString qtDir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (qtTranslator.load(QStringLiteral("qtbase_%1").arg(code), qtDir))
        app.installTranslator(&qtTranslator);
}

} // namespace amule::i18n
