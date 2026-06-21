// Client-side filter for search results: name match (plain substring, glob or
// regex, optionally negated) plus a complete-only flag and min/max source
// bounds. Ported from rstamule-cli's result_passes/build_filter_regex.

#pragma once

#include <QRegularExpression>
#include <QSet>
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

    // When enabled, the given source rows float to the top (anchored) regardless
    // of the active column sort.
    void setAnchored(const QSet<int>& sourceRows, bool enabled);

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    SearchFilter filter_;
    QRegularExpression pattern_;
    bool usePattern_ = false;

    QSet<int> anchored_;
    bool anchorEnabled_ = false;
};

} // namespace amule
