#include "searchresultmodel.h"

#include <algorithm>

namespace amule {

SearchResultModel::SearchResultModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int SearchResultModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(results_.size());
}

int SearchResultModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant SearchResultModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= results_.size())
        return {};
    const SearchResult& r = results_.at(index.row());

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == Size || index.column() == Sources)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }
    // Numeric/natural sort key (used by the proxy's sort role).
    if (role == Qt::UserRole) {
        switch (index.column()) {
        case Name:
            return r.name;
        case Size:
            return QVariant::fromValue<qulonglong>(r.size);
        case Sources:
            return QVariant::fromValue<qulonglong>(r.sourceCount);
        default:
            return {};
        }
    }
    if (role != Qt::DisplayRole)
        return {};

    switch (index.column()) {
    case Name:
        return r.name;
    case Size:
        return humanBytes(r.size);
    case Sources:
        return QStringLiteral("%1 (%2)").arg(r.sourceCount).arg(r.completeSourceCount);
    default:
        return {};
    }
}

QVariant SearchResultModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case Name:
        return QStringLiteral("Name");
    case Size:
        return QStringLiteral("Size");
    case Sources:
        return QStringLiteral("Sources (complete)");
    default:
        return {};
    }
}

Hash16 SearchResultModel::hashAt(int row) const {
    if (row < 0 || row >= results_.size())
        return {};
    return results_.at(row).hash;
}

SearchResult SearchResultModel::resultAt(int row) const {
    if (row < 0 || row >= results_.size())
        return {};
    return results_.at(row);
}

void SearchResultModel::setResults(const QList<SearchResult>& results) {
    // Update in place when the same results are listed in the same order, so a
    // live search refresh doesn't clear the table's selection.
    const bool sameLayout =
        results.size() == results_.size() &&
        std::equal(results.cbegin(), results.cend(), results_.cbegin(),
                   [](const SearchResult& a, const SearchResult& b) { return a.hash == b.hash; });
    if (sameLayout && !results.isEmpty()) {
        results_ = results;
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    } else {
        beginResetModel();
        results_ = results;
        endResetModel();
    }
}

} // namespace amule
