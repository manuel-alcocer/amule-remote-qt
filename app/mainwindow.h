// Main application window: a connection bar, a live download table driven by
// the worker's snapshots, a status/stats line and an activity log. The window
// never touches the socket — it talks to the EcWorker across the thread
// boundary (ADR-0004).

#pragma once

#include <QList>
#include <QMainWindow>

#include "ec/worker.h"

class QAction;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSortFilterProxyModel;
class QSpinBox;
class QTableView;

namespace amule {

class DownloadTableModel;
class PreferencesPanel;
class SearchPanel;
class ServerTableModel;
class SharedFileModel;
class SourceTableModel;
class SpeedGraph;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onTableContextMenu(const QPoint& pos);
    void onTransferSelectionChanged();
    void onServerContextMenu(const QPoint& pos);
    void onAddLink();
    void onAddServer();
    void onUpdateServers();

    void onStatusChanged(amule::ConnStatus status, const QString& detail);
    void onStats(amule::Stats stats);
    void onConnState(amule::ConnState conn);
    void onLog(const QString& message);

private:
    void buildUi();
    void buildToolBar();
    void wireWorker();
    void loadSettings();
    void saveSettings();
    [[nodiscard]] EcWorker* worker() const { return workerThread_.worker(); }

    EcWorkerThread workerThread_;
    DownloadTableModel* model_ = nullptr;

    QLineEdit* hostEdit_ = nullptr;
    QSpinBox* portSpin_ = nullptr;
    QLineEdit* passEdit_ = nullptr;
    QCheckBox* rememberCheck_ = nullptr;
    QPushButton* connectBtn_ = nullptr;
    QPushButton* disconnectBtn_ = nullptr;

    SpeedGraph* speedGraph_ = nullptr;
    QTableView* table_ = nullptr;
    QSortFilterProxyModel* downloadProxy_ = nullptr;
    SearchPanel* searchPanel_ = nullptr;
    PreferencesPanel* preferencesPanel_ = nullptr;
    QTableView* serverTable_ = nullptr;
    ServerTableModel* serverModel_ = nullptr;
    SharedFileModel* sharedModel_ = nullptr;
    SourceTableModel* sourceModel_ = nullptr;
    QPlainTextEdit* log_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* netLabel_ = nullptr;
    QLabel* statsLabel_ = nullptr;

    // Toolbar actions / widgets that require an active connection.
    QList<QAction*> connectedActions_;
    QList<QWidget*> connectedWidgets_;

    // Latest daemon preferences snapshot, used to seed the preferences dialog.
    DaemonPrefs lastPrefs_;

    // Path of the persisted speed-graph history.
    QString speedHistoryPath_;
};

} // namespace amule
