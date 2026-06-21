// Search tab: a query form with client-side filters (regex/glob/negate,
// complete-only, min/max sources), saved-search presets, and one closable
// session tab per executed search (lost on close). Public signals/slots are
// unchanged so MainWindow wiring stays the same.

#pragma once

#include <QList>
#include <QWidget>

#include "model/model.h"
#include "savedsearches.h"
#include "searchfilterproxy.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableView;
class QTabWidget;

namespace amule {

class SearchResultFilterProxy;
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
    void onSearch();
    void onDownload();
    void onTabClose(int index);
    void onCurrentTabChanged(int index);
    void onFilterChanged();
    void onSaveSearch();
    void onLoadSaved();
    void onDeleteSaved();

private:
    QWidget* buildControls();
    QTableView* makeResultTab(const QString& label);
    void onResultContextMenu(QTableView* view, const QPoint& pos);
    void showResultDetail(const amule::SearchResult& result);
    void updateAnchor(QTableView* view);
    [[nodiscard]] SearchFilter currentFilter() const;
    [[nodiscard]] QTableView* currentResultView() const;
    [[nodiscard]] SearchResultFilterProxy* proxyFor(QTableView* view) const;
    [[nodiscard]] SearchResultModel* modelFor(QTableView* view) const;
    [[nodiscard]] Hash16 selectedHash() const;
    void applyFilterToCurrent();
    void updateDownloadEnabled();
    void refreshSavedCombo();

    QLineEdit* queryEdit_ = nullptr;
    QComboBox* networkCombo_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QPushButton* searchBtn_ = nullptr;
    QPushButton* downloadBtn_ = nullptr;

    QLineEdit* filterEdit_ = nullptr;
    QCheckBox* negCheck_ = nullptr;
    QCheckBox* globCheck_ = nullptr;
    QCheckBox* regexCheck_ = nullptr;
    QCheckBox* onlyCompleteCheck_ = nullptr;
    QCheckBox* anchorCheck_ = nullptr;
    QSpinBox* minSrcSpin_ = nullptr;
    QSpinBox* maxSrcSpin_ = nullptr;

    QComboBox* savedCombo_ = nullptr;
    QPushButton* loadBtn_ = nullptr;
    QPushButton* saveBtn_ = nullptr;
    QPushButton* deleteBtn_ = nullptr;

    QProgressBar* progress_ = nullptr;
    QTabWidget* tabs_ = nullptr;

    QList<SavedSearch> saved_;

    // The most-recent search tab — the only one whose results live in the
    // daemon and are therefore downloadable.
    QTableView* liveView_ = nullptr;
    SearchResultModel* liveModel_ = nullptr;
};

} // namespace amule
