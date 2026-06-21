// Table model exposing the download queue to a QTableView. Refreshed wholesale
// from the worker's downloadsUpdated snapshots (a few hundred rows at ~1 Hz).

#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "model/model.h"

namespace amule {

class DownloadTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Name, Status, Progress, Size, Speed, Sources, ColumnCount };

    // Custom role carrying the integer percent (0..100) for the progress
    // delegate.
    static constexpr int ProgressRole = Qt::UserRole + 1;

    explicit DownloadTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    // Hash of the download at `row`, for issuing commands.
    [[nodiscard]] Hash16 hashAt(int row) const;

public slots:
    void setDownloads(const QList<amule::Download>& downloads);

private:
    QList<Download> downloads_;
};

} // namespace amule
