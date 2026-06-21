// Main application window: a connection bar, a live download table driven by
// the worker's snapshots, a status/stats line and an activity log. The window
// never touches the socket — it talks to the EcWorker across the thread
// boundary (ADR-0004).

#pragma once

#include <QMainWindow>

#include "ec/worker.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableView;

namespace amule {

class DownloadTableModel;
class SearchPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onTableContextMenu(const QPoint& pos);

    void onStatusChanged(amule::ConnStatus status, const QString& detail);
    void onStats(amule::Stats stats);
    void onConnState(amule::ConnState conn);
    void onLog(const QString& message);

private:
    void buildUi();
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

    QTableView* table_ = nullptr;
    SearchPanel* searchPanel_ = nullptr;
    QPlainTextEdit* log_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* netLabel_ = nullptr;
    QLabel* statsLabel_ = nullptr;
};

} // namespace amule
