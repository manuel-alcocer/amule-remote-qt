// Shared helper for live tests: load amuled credentials ONLY from the
// environment or the external ~/.config/rstamule/config.ini. Never embeds
// secrets in the repository. Returns ok=false when nothing is configured so
// callers can QSKIP.

#pragma once

#include <QDir>
#include <QFile>
#include <QHash>
#include <QString>
#include <QtTypes>

namespace test {

struct Credentials {
    QString host;
    quint16 port = 4712;
    QString passwordMd5;   // preferred (the form amuled stores)
    QString passwordPlain; // used only if no MD5 digest is available
    bool ok = false;
};

inline QHash<QString, QString> parseSimpleConfig(const QString& path) {
    QHash<QString, QString> out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return out;
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq > 0)
            out.insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
    }
    return out;
}

inline Credentials loadCredentials() {
    Credentials c;
    const QString cfgPath =
        QDir::homePath() + QStringLiteral("/.config/rstamule/config.ini");
    const QHash<QString, QString> cfg = parseSimpleConfig(cfgPath);

    c.host = qEnvironmentVariable("AMULE_HOST", cfg.value(QStringLiteral("host")));
    const QString portStr =
        qEnvironmentVariable("AMULE_PORT", cfg.value(QStringLiteral("port")));
    if (!portStr.isEmpty())
        c.port = static_cast<quint16>(portStr.toUInt());
    c.passwordMd5 = qEnvironmentVariable("AMULE_PASSWORD_MD5",
                                         cfg.value(QStringLiteral("password_md5")));
    c.passwordPlain = qEnvironmentVariable("AMULE_PASSWORD", QString());

    c.ok = !c.host.isEmpty() &&
           (!c.passwordMd5.isEmpty() || !c.passwordPlain.isEmpty());
    return c;
}

} // namespace test
