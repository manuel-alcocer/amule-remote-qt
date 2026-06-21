#include "servertablemodel.h"

#include <algorithm>

namespace amule {

ServerTableModel::ServerTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int ServerTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(servers_.size());
}

int ServerTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ServerTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= servers_.size())
        return {};
    const Server& s = servers_.at(index.row());

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == Users || index.column() == Files ||
            index.column() == Ping)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role == Qt::UserRole) {
        switch (index.column()) {
        case Users:
            return QVariant::fromValue<qulonglong>(s.users);
        case Files:
            return QVariant::fromValue<qulonglong>(s.files);
        case Ping:
            return QVariant::fromValue<qulonglong>(s.ping);
        default:
            break;
        }
    }
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case Name:
        return s.name.isEmpty() ? s.address() : s.name;
    case Address:
        return QStringLiteral("%1:%2").arg(s.address()).arg(s.port);
    case Users:
        return humanCount(s.users);
    case Files:
        return humanCount(s.files);
    case Ping:
        return tr("%1 ms").arg(s.ping);
    case Priority:
        return s.priorityLabel();
    default:
        return {};
    }
}

QVariant ServerTableModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Name:
        return tr("Name");
    case Address:
        return tr("Address");
    case Users:
        return tr("Users");
    case Files:
        return tr("Files");
    case Ping:
        return tr("Ping");
    case Priority:
        return tr("Priority");
    default:
        return {};
    }
}

ServerIp ServerTableModel::ipAt(int row) const {
    if (row < 0 || row >= servers_.size())
        return {};
    return servers_.at(row).ip;
}

quint16 ServerTableModel::portAt(int row) const {
    if (row < 0 || row >= servers_.size())
        return 0;
    return servers_.at(row).port;
}

void ServerTableModel::setServers(const QList<Server>& servers) {
    // Update in place when the same servers are listed in the same order, so a
    // periodic refresh doesn't clear the table's selection.
    const bool sameLayout =
        servers.size() == servers_.size() &&
        std::equal(servers.cbegin(), servers.cend(), servers_.cbegin(),
                   [](const Server& a, const Server& b) {
                       return a.ip == b.ip && a.port == b.port;
                   });
    if (sameLayout && !servers.isEmpty()) {
        servers_ = servers;
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    } else {
        beginResetModel();
        servers_ = servers;
        endResetModel();
    }
}

} // namespace amule
