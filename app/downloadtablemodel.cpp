#include "downloadtablemodel.h"

namespace amule {

DownloadTableModel::DownloadTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int DownloadTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(downloads_.size());
}

int DownloadTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant DownloadTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= downloads_.size())
        return {};
    const Download& d = downloads_.at(index.row());

    if (role == ProgressRole)
        return static_cast<int>(d.progress() * 100.0F);

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
        case Size:
        case Speed:
        case Sources:
            return int(Qt::AlignRight | Qt::AlignVCenter);
        default:
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case Name:
        return d.name;
    case Status:
        return d.statusLabel();
    case Progress:
        return QStringLiteral("%1%").arg(d.progress() * 100.0F, 0, 'f', 1);
    case Size:
        return humanSizePair(d.sizeDone, d.sizeFull);
    case Speed:
        return d.speed > 0 ? humanRate(d.speed) : QStringLiteral("—");
    case Sources:
        return QStringLiteral("%1 (%2)").arg(d.sourceCount).arg(d.sourceCountXfer);
    default:
        return {};
    }
}

QVariant DownloadTableModel::headerData(int section, Qt::Orientation orientation,
                                        int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Name:
        return QStringLiteral("Name");
    case Status:
        return QStringLiteral("Status");
    case Progress:
        return QStringLiteral("Progress");
    case Size:
        return QStringLiteral("Size");
    case Speed:
        return QStringLiteral("Speed");
    case Sources:
        return QStringLiteral("Sources");
    default:
        return {};
    }
}

Hash16 DownloadTableModel::hashAt(int row) const {
    if (row < 0 || row >= downloads_.size())
        return {};
    return downloads_.at(row).hash;
}

void DownloadTableModel::setDownloads(const QList<Download>& downloads) {
    beginResetModel();
    downloads_ = downloads;
    endResetModel();
}

} // namespace amule
