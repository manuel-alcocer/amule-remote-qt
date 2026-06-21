// Table model exposing the eD2k server list to a QTableView.

#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "model/model.h"

namespace amule {

class ServerTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Name, Address, Users, Files, Ping, Priority, ColumnCount };

    explicit ServerTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;

    [[nodiscard]] ServerIp ipAt(int row) const;
    [[nodiscard]] quint16 portAt(int row) const;

public slots:
    void setServers(const QList<amule::Server>& servers);

private:
    QList<Server> servers_;
};

} // namespace amule
