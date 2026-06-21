#include "downloadtablemodel.h"

#include <algorithm>

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

    // Numeric sort keys so columns with units (Size, Speed, …) sort by value,
    // not by their formatted text.
    if (role == Qt::UserRole) {
        switch (index.column()) {
        case PartNum:
            return d.partMetId;
        case Name:
            return d.name;
        case Status:
            return d.statusLabel();
        case Progress:
            return d.progress();
        case Size:
            return QVariant::fromValue<qulonglong>(d.sizeFull);
        case Speed:
            return QVariant::fromValue<qulonglong>(d.speed);
        case Sources:
            return QVariant::fromValue<qulonglong>(d.sourceCount);
        default:
            return {};
        }
    }

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
        case PartNum:
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
    case PartNum:
        return d.partMetId > 0 ? QString::number(d.partMetId) : QStringLiteral("—");
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
    case PartNum:
        return tr("Part#");
    case Name:
        return tr("Name");
    case Status:
        return tr("Status");
    case Progress:
        return tr("Progress");
    case Size:
        return tr("Size");
    case Speed:
        return tr("Speed");
    case Sources:
        return tr("Sources");
    default:
        return {};
    }
}

Hash16 DownloadTableModel::hashAt(int row) const {
    if (row < 0 || row >= downloads_.size())
        return {};
    return downloads_.at(row).hash;
}

quint32 DownloadTableModel::ecidAt(int row) const {
    if (row < 0 || row >= downloads_.size())
        return 0;
    return downloads_.at(row).ecid;
}

void DownloadTableModel::setDownloads(const QList<Download>& downloads) {
    // When the queue has the same rows in the same order (the common case on a
    // periodic refresh), update values in place and emit dataChanged rather than
    // resetting the model — a reset would clear the view's selection and current
    // index every tick.
    const bool sameLayout =
        downloads.size() == downloads_.size() &&
        std::equal(downloads.cbegin(), downloads.cend(), downloads_.cbegin(),
                   [](const Download& a, const Download& b) { return a.hash == b.hash; });

    if (sameLayout && !downloads.isEmpty()) {
        downloads_ = downloads;
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    } else {
        beginResetModel();
        downloads_ = downloads;
        endResetModel();
    }
}

} // namespace amule
