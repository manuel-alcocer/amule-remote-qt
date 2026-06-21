// Table model exposing the source clients of the currently-selected download.
// Holds every source the worker reports and shows only those whose fileEcid
// matches the selected download's ecid.

#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "model/model.h"

namespace amule {

class SourceTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Client, Address, Speed, Downloaded, State, Origin, ColumnCount };

    explicit SourceTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

public slots:
    void setSources(const QList<amule::SourceClient>& sources);
    void setFileEcid(quint32 ecid);

private:
    void rebuild();

    QList<SourceClient> all_;
    QList<SourceClient> visible_;
    quint32 fileEcid_ = 0;
};

} // namespace amule
