#include "sourcetablemodel.h"

#include <algorithm>

namespace amule {

SourceTableModel::SourceTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int SourceTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(visible_.size());
}

int SourceTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant SourceTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= visible_.size())
        return {};
    const SourceClient& c = visible_.at(index.row());

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == Speed || index.column() == Downloaded)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case Client:
        return c.name.isEmpty() ? c.softVer : c.name;
    case Address:
        return QStringLiteral("%1:%2").arg(c.ipString()).arg(c.userPort);
    case Speed:
        return c.downSpeed > 0.0
                   ? humanRate(static_cast<quint64>(c.downSpeed))
                   : QStringLiteral("—");
    case Downloaded:
        return humanBytes(c.downloaded);
    case State:
        return c.stateLabel();
    case Origin:
        return c.originLabel();
    default:
        return {};
    }
}

QVariant SourceTableModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Client:
        return QStringLiteral("Client");
    case Address:
        return QStringLiteral("Address");
    case Speed:
        return QStringLiteral("Speed");
    case Downloaded:
        return QStringLiteral("Downloaded");
    case State:
        return QStringLiteral("State");
    case Origin:
        return QStringLiteral("Origin");
    default:
        return {};
    }
}

void SourceTableModel::setSources(const QList<SourceClient>& sources) {
    all_ = sources;
    rebuild();
}

void SourceTableModel::setFileEcid(quint32 ecid) {
    if (fileEcid_ == ecid)
        return;
    fileEcid_ = ecid;
    rebuild();
}

void SourceTableModel::rebuild() {
    QList<SourceClient> next;
    if (fileEcid_ != 0) {
        for (const SourceClient& c : all_) {
            if (c.fileEcid == fileEcid_)
                next.append(c);
        }
    }
    // Update in place when the same sources are listed in the same order, so a
    // periodic refresh doesn't clear the selection.
    const bool sameLayout =
        next.size() == visible_.size() &&
        std::equal(next.cbegin(), next.cend(), visible_.cbegin(),
                   [](const SourceClient& a, const SourceClient& b) { return a.id == b.id; });
    if (sameLayout && !next.isEmpty()) {
        visible_ = next;
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    } else {
        beginResetModel();
        visible_ = next;
        endResetModel();
    }
}

} // namespace amule
