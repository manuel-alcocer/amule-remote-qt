// Table model exposing search results to a QTableView.

#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "model/model.h"

namespace amule {

class SearchResultModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Name, Size, Sources, ColumnCount };

    explicit SearchResultModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    [[nodiscard]] Hash16 hashAt(int row) const;

public slots:
    void setResults(const QList<amule::SearchResult>& results);

private:
    QList<SearchResult> results_;
};

} // namespace amule
