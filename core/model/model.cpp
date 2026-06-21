#include "model/model.h"

#include <array>

#include <QCoreApplication>

#include "ec/codes.h"

namespace amule {

QString Download::statusLabel() const {
    using namespace amule::ec::status;
    switch (status) {
    case READY:
        return QCoreApplication::translate("Model", "Downloading");
    case EMPTY:
        return QCoreApplication::translate("Model", "Empty");
    case WAITING_FOR_HASH:
        return QCoreApplication::translate("Model", "Waiting hash");
    case HASHING:
        return QCoreApplication::translate("Model", "Hashing");
    case ERROR:
        return QCoreApplication::translate("Model", "Error");
    case INSUFFICIENT:
        return QCoreApplication::translate("Model", "Insufficient");
    case PAUSED:
        return QCoreApplication::translate("Model", "Paused");
    case COMPLETING:
        return QCoreApplication::translate("Model", "Completing");
    case COMPLETE:
        return QCoreApplication::translate("Model", "Complete");
    case ALLOC:
        return QCoreApplication::translate("Model", "Allocating");
    default:
        return QCoreApplication::translate("Model", "Unknown");
    }
}

QString SourceClient::ipString() const {
    return QStringLiteral("%1.%2.%3.%4")
        .arg((userIp >> 24) & 0xFF)
        .arg((userIp >> 16) & 0xFF)
        .arg((userIp >> 8) & 0xFF)
        .arg(userIp & 0xFF);
}

QString SourceClient::stateLabel() const {
    switch (downState & 0xFF) {
    case 0:
        return QCoreApplication::translate("Model", "Downloading");
    case 1:
        return QCoreApplication::translate("Model", "On Queue");
    case 2:
        return QCoreApplication::translate("Model", "Connecting");
    case 3:
        return QCoreApplication::translate("Model", "Requesting");
    case 4:
        return QCoreApplication::translate("Model", "No needed parts");
    case 5:
        return QCoreApplication::translate("Model", "Connecting via server");
    case 6:
        return QCoreApplication::translate("Model", "Too many connections");
    case 7:
        return QCoreApplication::translate("Model", "Connecting via Kad");
    case 8:
        return QCoreApplication::translate("Model", "Asked for download");
    default:
        return QStringLiteral("—");
    }
}

QString SourceClient::originLabel() const {
    switch (origin) {
    case 0:
        return QCoreApplication::translate("Model", "Local Server");
    case 1:
        return QCoreApplication::translate("Model", "Remote Server");
    case 2:
        return QCoreApplication::translate("Model", "Kad");
    case 3:
        return QCoreApplication::translate("Model", "Source Exchange");
    case 4:
        return QCoreApplication::translate("Model", "Passive");
    case 5:
        return QCoreApplication::translate("Model", "Link");
    case 6:
        return QCoreApplication::translate("Model", "Source Seeds");
    case 7:
        return QCoreApplication::translate("Model", "Search Result");
    default:
        return QStringLiteral("—");
    }
}

quint64 searchKindValue(SearchKind kind) {
    switch (kind) {
    case SearchKind::Global:
        return ec::EC_SEARCH_GLOBAL;
    case SearchKind::Kad:
        return ec::EC_SEARCH_KAD;
    case SearchKind::Local:
        return ec::EC_SEARCH_LOCAL;
    }
    return ec::EC_SEARCH_GLOBAL;
}

QString Server::address() const {
    return QStringLiteral("%1.%2.%3.%4")
        .arg(ip[0])
        .arg(ip[1])
        .arg(ip[2])
        .arg(ip[3]);
}

QString Server::priorityLabel() const {
    switch (priority) {
    case 0:
        return QCoreApplication::translate("Model", "Normal");
    case 1:
        return QCoreApplication::translate("Model", "High");
    case 2:
        return QCoreApplication::translate("Model", "Low");
    default:
        return QStringLiteral("?");
    }
}

// --- Formatting helpers ----------------------------------------------------

namespace {
constexpr std::array<const char*, 6> kSizeUnits = {"B",   "KiB", "MiB",
                                                   "GiB", "TiB", "PiB"};
} // namespace

QString humanBytes(quint64 n) {
    double v = static_cast<double>(n);
    int i = 0;
    while (v >= 1024.0 && i < static_cast<int>(kSizeUnits.size()) - 1) {
        v /= 1024.0;
        ++i;
    }
    if (i == 0)
        return QStringLiteral("%1 B").arg(n);
    return QStringLiteral("%1 %2").arg(v, 0, 'f', 2).arg(
        QString::fromLatin1(kSizeUnits[i]));
}

QString humanRate(quint64 n) {
    return humanBytes(n) + QStringLiteral("/s");
}

QString humanCount(quint64 n) {
    struct Unit {
        const char* suffix;
        double div;
    };
    static constexpr Unit kUnits[] = {{"G", 1e9}, {"M", 1e6}, {"k", 1e3}};
    const auto value = static_cast<double>(n);
    for (const Unit& u : kUnits) {
        if (value >= u.div)
            return QStringLiteral("%1%2").arg(value / u.div, 0, 'f', 1).arg(
                QString::fromLatin1(u.suffix));
    }
    return QString::number(n);
}

QString humanSizePair(quint64 done, quint64 full) {
    double v = static_cast<double>(full);
    int i = 0;
    double div = 1.0;
    while (v >= 1024.0 && i < static_cast<int>(kSizeUnits.size()) - 1) {
        v /= 1024.0;
        div *= 1024.0;
        ++i;
    }
    const auto fmt = [&](double x) -> QString {
        if (i == 0)
            return QString::number(static_cast<quint64>(x));
        if (x >= 100.0)
            return QString::number(x, 'f', 0);
        return QString::number(x, 'f', 1);
    };
    return QStringLiteral("%1 / %2 %3")
        .arg(fmt(static_cast<double>(done) / div))
        .arg(fmt(v))
        .arg(QString::fromLatin1(kSizeUnits[i]));
}

} // namespace amule
