#include "mainwindow.h"

#include <QCheckBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QTableView>
#include <QVBoxLayout>

#include "downloadtablemodel.h"
#include "ec/client.h"
#include "progressbardelegate.h"

namespace amule {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    wireWorker();
    loadSettings();
    onStatusChanged(ConnStatus::Disconnected, QString());
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    // Connection bar.
    auto* bar = new QHBoxLayout;
    hostEdit_ = new QLineEdit;
    hostEdit_->setPlaceholderText(QStringLiteral("host"));
    portSpin_ = new QSpinBox;
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(4712);
    passEdit_ = new QLineEdit;
    passEdit_->setPlaceholderText(QStringLiteral("password"));
    passEdit_->setEchoMode(QLineEdit::Password);
    rememberCheck_ = new QCheckBox(QStringLiteral("Remember"));
    connectBtn_ = new QPushButton(QStringLiteral("Connect"));
    disconnectBtn_ = new QPushButton(QStringLiteral("Disconnect"));

    bar->addWidget(new QLabel(QStringLiteral("Host:")));
    bar->addWidget(hostEdit_, 2);
    bar->addWidget(new QLabel(QStringLiteral("Port:")));
    bar->addWidget(portSpin_);
    bar->addWidget(new QLabel(QStringLiteral("Pass:")));
    bar->addWidget(passEdit_, 1);
    bar->addWidget(rememberCheck_);
    bar->addWidget(connectBtn_);
    bar->addWidget(disconnectBtn_);
    layout->addLayout(bar);

    // Download table.
    model_ = new DownloadTableModel(this);
    table_ = new QTableView;
    table_->setModel(model_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->verticalHeader()->setVisible(false);
    table_->setItemDelegateForColumn(DownloadTableModel::Progress,
                                     new ProgressBarDelegate(this));
    table_->setContextMenuPolicy(Qt::CustomContextMenu);
    auto* header = table_->horizontalHeader();
    header->setSectionResizeMode(DownloadTableModel::Name, QHeaderView::Stretch);
    for (int c = DownloadTableModel::Status; c < DownloadTableModel::ColumnCount; ++c)
        header->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    layout->addWidget(table_, 1);

    setCentralWidget(central);

    // Activity log dock.
    auto* dock = new QDockWidget(QStringLiteral("Log"), this);
    log_ = new QPlainTextEdit;
    log_->setReadOnly(true);
    log_->setMaximumBlockCount(500);
    dock->setWidget(log_);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    // Status bar.
    statusLabel_ = new QLabel;
    netLabel_ = new QLabel;
    statsLabel_ = new QLabel;
    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->addPermanentWidget(netLabel_);
    statusBar()->addPermanentWidget(statsLabel_);

    connect(connectBtn_, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectBtn_, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(passEdit_, &QLineEdit::returnPressed, this, &MainWindow::onConnectClicked);
    connect(table_, &QTableView::customContextMenuRequested, this,
            &MainWindow::onTableContextMenu);
}

void MainWindow::wireWorker() {
    EcWorker* w = worker();
    // Queued automatically: the worker lives on another thread.
    connect(w, &EcWorker::statusChanged, this, &MainWindow::onStatusChanged);
    connect(w, &EcWorker::statsUpdated, this, &MainWindow::onStats);
    connect(w, &EcWorker::connStateUpdated, this, &MainWindow::onConnState);
    connect(w, &EcWorker::logMessage, this, &MainWindow::onLog);
    connect(w, &EcWorker::downloadsUpdated, model_, &DownloadTableModel::setDownloads);
}

void MainWindow::onConnectClicked() {
    const QString host = hostEdit_->text().trimmed();
    if (host.isEmpty()) {
        onLog(QStringLiteral("enter a host first"));
        return;
    }
    const auto port = static_cast<quint16>(portSpin_->value());
    const QString password = passEdit_->text();
    saveSettings();
    QMetaObject::invokeMethod(worker(), "connectToDaemon", Qt::QueuedConnection,
                              Q_ARG(QString, host), Q_ARG(quint16, port),
                              Q_ARG(QString, password));
}

void MainWindow::onDisconnectClicked() {
    QMetaObject::invokeMethod(worker(), "disconnectFromDaemon", Qt::QueuedConnection);
}

void MainWindow::onTableContextMenu(const QPoint& pos) {
    const QModelIndex index = table_->indexAt(pos);
    if (!index.isValid())
        return;
    const Hash16 hash = model_->hashAt(index.row());

    QMenu menu(this);
    QAction* pause = menu.addAction(QStringLiteral("Pause"));
    QAction* resume = menu.addAction(QStringLiteral("Resume"));
    QAction* stop = menu.addAction(QStringLiteral("Stop"));
    menu.addSeparator();
    QAction* remove = menu.addAction(QStringLiteral("Delete…"));

    QAction* chosen = menu.exec(table_->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    const char* slot = nullptr;
    if (chosen == pause)
        slot = "pause";
    else if (chosen == resume)
        slot = "resume";
    else if (chosen == stop)
        slot = "stop";
    else if (chosen == remove) {
        if (QMessageBox::question(
                this, QStringLiteral("Delete download"),
                QStringLiteral("Delete this download from the queue?")) !=
            QMessageBox::Yes)
            return;
        slot = "remove";
    }
    if (slot)
        QMetaObject::invokeMethod(worker(), slot, Qt::QueuedConnection,
                                  Q_ARG(amule::Hash16, hash));
}

void MainWindow::onStatusChanged(ConnStatus status, const QString& detail) {
    bool connected = false;
    bool connecting = false;
    switch (status) {
    case ConnStatus::Disconnected:
        statusLabel_->setText(QStringLiteral("Disconnected"));
        break;
    case ConnStatus::Connecting:
        statusLabel_->setText(QStringLiteral("Connecting to %1…").arg(detail));
        connecting = true;
        break;
    case ConnStatus::Connected:
        statusLabel_->setText(QStringLiteral("Connected — daemon %1").arg(detail));
        connected = true;
        break;
    case ConnStatus::Error:
        statusLabel_->setText(QStringLiteral("Error: %1").arg(detail));
        break;
    }
    connectBtn_->setEnabled(!connected && !connecting);
    disconnectBtn_->setEnabled(connected);
    if (!connected) {
        netLabel_->clear();
        statsLabel_->clear();
    }
}

void MainWindow::onStats(Stats stats) {
    statsLabel_->setText(QStringLiteral("↓ %1   ↑ %2   ed2k %3 users%4")
                             .arg(humanRate(stats.dlSpeed), humanRate(stats.ulSpeed),
                                  humanCount(stats.ed2kUsers),
                                  stats.isHighId() ? QStringLiteral("   HighID")
                                                   : QString()));
}

void MainWindow::onConnState(ConnState conn) {
    const QString ed2k = conn.ed2kConnected
                             ? QStringLiteral("ed2k: %1").arg(conn.ed2kServer)
                             : QStringLiteral("ed2k: off");
    const QString kad =
        conn.kadConnected ? QStringLiteral("Kad: on") : QStringLiteral("Kad: off");
    netLabel_->setText(QStringLiteral("%1   |   %2").arg(ed2k, kad));
}

void MainWindow::onLog(const QString& message) {
    log_->appendPlainText(message);
}

void MainWindow::loadSettings() {
    QSettings settings;
    hostEdit_->setText(settings.value(QStringLiteral("host"),
                                      QStringLiteral("127.0.0.1")).toString());
    portSpin_->setValue(settings.value(QStringLiteral("port"), 4712).toInt());
    const bool remember = settings.value(QStringLiteral("remember"), false).toBool();
    rememberCheck_->setChecked(remember);
    if (remember) {
        // Stored as an MD5 digest (ADR-0006); it is accepted as-is on connect.
        passEdit_->setText(settings.value(QStringLiteral("password_md5")).toString());
    }
}

void MainWindow::saveSettings() {
    QSettings settings;
    settings.setValue(QStringLiteral("host"), hostEdit_->text().trimmed());
    settings.setValue(QStringLiteral("port"), portSpin_->value());
    settings.setValue(QStringLiteral("remember"), rememberCheck_->isChecked());
    if (rememberCheck_->isChecked()) {
        // Never store the plaintext password — only its MD5 digest (ADR-0006).
        settings.setValue(QStringLiteral("password_md5"),
                          ec::normalizePasswordMd5(passEdit_->text()));
    } else {
        settings.remove(QStringLiteral("password_md5"));
    }
}

} // namespace amule
