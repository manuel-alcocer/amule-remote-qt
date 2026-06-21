// Search tab: query controls, a results table and actions to enqueue a result.
// Emits intent signals; MainWindow forwards them to the worker.

#pragma once

#include <QList>
#include <QWidget>

#include "model/model.h"

class QComboBox;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSortFilterProxyModel;
class QTableView;

namespace amule {

class SearchResultModel;

class SearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(QWidget* parent = nullptr);

public slots:
    void setResults(const QList<amule::SearchResult>& results);
    void setProgress(quint32 progress);
    void setSearching(bool searching);

signals:
    void searchRequested(amule::SearchParams params);
    void stopRequested();
    void downloadRequested(amule::Hash16 hash);

private slots:
    void onSearchOrStop();
    void onDownloadSelected();

private:
    [[nodiscard]] Hash16 selectedHash() const;

    QLineEdit* queryEdit_ = nullptr;
    QComboBox* networkCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QPushButton* searchBtn_ = nullptr;
    QPushButton* downloadBtn_ = nullptr;
    QProgressBar* progress_ = nullptr;
    QTableView* table_ = nullptr;
    SearchResultModel* model_ = nullptr;
    QSortFilterProxyModel* proxy_ = nullptr;
    bool searching_ = false;
};

} // namespace amule
