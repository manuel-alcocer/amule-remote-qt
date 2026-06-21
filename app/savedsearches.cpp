#include "savedsearches.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace amule {
namespace {

QString storePath() {
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return dir + QStringLiteral("/searches.tsv");
}

QString clean(const QString& s) {
    QString out = s;
    out.replace(QLatin1Char('\t'), QLatin1Char(' '));
    out.replace(QLatin1Char('\n'), QLatin1Char(' '));
    out.replace(QLatin1Char('\r'), QLatin1Char(' '));
    return out;
}

QString boolField(bool v) {
    return v ? QStringLiteral("1") : QStringLiteral("0");
}

} // namespace

QList<SavedSearch> loadSavedSearches() {
    QList<SavedSearch> out;
    QFile file(storePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty())
            continue;
        const QStringList f = line.split(QLatin1Char('\t'));
        if (f.size() != 11)
            continue;
        SavedSearch s;
        s.name = f[0];
        s.text = f[1];
        s.kindIndex = f[2].toInt();
        s.typeIndex = f[3].toInt();
        s.filterText = f[4];
        s.filterNegate = f[5] == QLatin1Char('1');
        s.filterGlob = f[6] == QLatin1Char('1');
        s.filterRegex = f[7] == QLatin1Char('1');
        s.onlyComplete = f[8] == QLatin1Char('1');
        s.minSources = f[9].toULongLong();
        s.maxSources = f[10].toULongLong();
        out.append(s);
    }
    return out;
}

void saveSavedSearches(const QList<SavedSearch>& searches) {
    const QString path = storePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QString text;
    for (const SavedSearch& s : searches) {
        const QStringList fields = {
            clean(s.name),
            clean(s.text),
            QString::number(s.kindIndex),
            QString::number(s.typeIndex),
            clean(s.filterText),
            boolField(s.filterNegate),
            boolField(s.filterGlob),
            boolField(s.filterRegex),
            boolField(s.onlyComplete),
            QString::number(s.minSources),
            QString::number(s.maxSources),
        };
        text += fields.join(QLatin1Char('\t'));
        text += QLatin1Char('\n');
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(text.toUtf8());
    file.commit();
}

} // namespace amule
