#include "sharedfilemodel.h"

#include <algorithm>

namespace amule {

SharedFileModel::SharedFileModel(QObject* parent) : QAbstractTableModel(parent) {}

int SharedFileModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(files_.size());
}

int SharedFileModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant SharedFileModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= files_.size())
        return {};
    const SharedFile& f = files_.at(index.row());

    if (role == Qt::TextAlignmentRole) {
        if (index.column() != Name)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role == Qt::UserRole) {
        switch (index.column()) {
        case Size:
            return QVariant::fromValue<qulonglong>(f.size);
        case Requests:
            return QVariant::fromValue<qulonglong>(f.requestCount);
        case Accepted:
            return QVariant::fromValue<qulonglong>(f.acceptCount);
        case Transferred:
            return QVariant::fromValue<qulonglong>(f.transferred);
        case Sources:
            return QVariant::fromValue<qulonglong>(f.completeSources);
        default:
            break;
        }
    }
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case Name:
        return f.name;
    case Size:
        return humanBytes(f.size);
    case Requests:
        return QString::number(f.requestCount);
    case Accepted:
        return QString::number(f.acceptCount);
    case Transferred:
        return humanBytes(f.transferred);
    case Sources:
        return QString::number(f.completeSources);
    default:
        return {};
    }
}

QVariant SharedFileModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Name:
        return tr("Name");
    case Size:
        return tr("Size");
    case Requests:
        return tr("Requests");
    case Accepted:
        return tr("Accepted");
    case Transferred:
        return tr("Transferred");
    case Sources:
        return tr("Sources");
    default:
        return {};
    }
}

void SharedFileModel::setFiles(const QList<SharedFile>& files) {
    // Update in place when the same files are listed in the same order, so a
    // periodic refresh doesn't clear the table's selection.
    const bool sameLayout =
        files.size() == files_.size() &&
        std::equal(files.cbegin(), files.cend(), files_.cbegin(),
                   [](const SharedFile& a, const SharedFile& b) { return a.hash == b.hash; });
    if (sameLayout && !files.isEmpty()) {
        files_ = files;
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    } else {
        beginResetModel();
        files_ = files;
        endResetModel();
    }
}

} // namespace amule
