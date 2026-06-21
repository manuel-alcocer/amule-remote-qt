// Application translations. To add a language: add it to languages(), drop an
// amule-remote-qt_<code>.ts under app/i18n (lupdate), translate it, release it
// to .qm (lrelease) and list the .qm in i18n.qrc. Everything else just works.

#pragma once

#include <QList>
#include <QString>

class QApplication;

namespace amule::i18n {

struct Language {
    QString code;       // e.g. "es" (empty is never used here; "" means system)
    QString nativeName; // e.g. "Español"
};

// Languages we ship a translation for. English is the source language.
QList<Language> languages();

// Persisted language setting: "" = follow the system locale, else a code.
QString currentSetting();
void setSetting(const QString& code);

// Load and install the translators for the current setting (call once at start).
void install(QApplication& app);

} // namespace amule::i18n
