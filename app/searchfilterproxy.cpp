#include "searchfilterproxy.h"

#include "searchresultmodel.h"

namespace amule {

QString globToRegex(const QString& glob) {
    QString re;
    re.reserve(glob.size() + 2);
    re.append(QLatin1Char('^'));
    for (const QChar ch : glob) {
        switch (ch.unicode()) {
        case u'*':
            re.append(QStringLiteral(".*"));
            break;
        case u'?':
            re.append(QLatin1Char('.'));
            break;
        case u'.':
        case u'+':
        case u'(':
        case u')':
        case u'|':
        case u'[':
        case u']':
        case u'{':
        case u'}':
        case u'^':
        case u'$':
        case u'\\':
            re.append(QLatin1Char('\\'));
            re.append(ch);
            break;
        default:
            re.append(ch);
        }
    }
    re.append(QLatin1Char('$'));
    return re;
}

void SearchResultFilterProxy::setSearchFilter(const SearchFilter& filter) {
    beginFilterChange();
    filter_ = filter;
    const QString text = filter_.text.trimmed();

    usePattern_ = false;
    if (!text.isEmpty() && (filter_.regex || filter_.glob)) {
        const QString expr = filter_.regex ? text : globToRegex(text);
        pattern_ = QRegularExpression(
            expr, QRegularExpression::CaseInsensitiveOption);
        usePattern_ = pattern_.isValid();
    }
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
}

bool SearchResultFilterProxy::filterAcceptsRow(int sourceRow,
                                               const QModelIndex& sourceParent) const {
    Q_UNUSED(sourceParent);
    const auto* model = qobject_cast<SearchResultModel*>(sourceModel());
    if (!model)
        return true;
    const SearchResult r = model->resultAt(sourceRow);

    if (filter_.onlyComplete && r.completeSourceCount == 0)
        return false;
    if (filter_.minSources > 0 && r.sourceCount < filter_.minSources)
        return false;
    if (filter_.maxSources > 0 && r.sourceCount > filter_.maxSources)
        return false;

    const QString text = filter_.text.trimmed();
    if (!text.isEmpty()) {
        bool match = usePattern_ ? pattern_.match(r.name).hasMatch()
                                 : r.name.contains(text, Qt::CaseInsensitive);
        if (filter_.negate)
            match = !match;
        if (!match)
            return false;
    }
    return true;
}

} // namespace amule
