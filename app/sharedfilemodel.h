// Table model exposing the daemon's shared files to a QTableView.

#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "model/model.h"

namespace amule {

class SharedFileModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Name, Size, Requests, Accepted, Transferred, Sources, ColumnCount };

    explicit SharedFileModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

public slots:
    void setFiles(const QList<amule::SharedFile>& files);

private:
    QList<SharedFile> files_;
};

} // namespace amule
