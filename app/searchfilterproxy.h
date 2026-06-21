// Client-side filter for search results: name match (plain substring, glob or
// regex, optionally negated) plus a complete-only flag and min/max source
// bounds. Ported from rstamule-cli's result_passes/build_filter_regex.

#pragma once

#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QString>
#include <QtTypes>

namespace amule {

struct SearchFilter {
    QString text;
    bool negate = false;
    bool glob = false;
    bool regex = false;
    bool onlyComplete = false;
    quint64 minSources = 0; // 0 = no lower bound
    quint64 maxSources = 0; // 0 = no upper bound
};

// Translate a glob (*, ?) into an anchored regex pattern.
QString globToRegex(const QString& glob);

class SearchResultFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT

public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setSearchFilter(const SearchFilter& filter);

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex& sourceParent) const override;

private:
    SearchFilter filter_;
    QRegularExpression pattern_;
    bool usePattern_ = false;
};

} // namespace amule
