#include "mainwindow.h"

#ifndef APP_VERSION
#define APP_VERSION "0.1.0"
#endif

#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QDir>
#include <QDockWidget>
#include <QMenuBar>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCloseEvent>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStandardPaths>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTabWidget>
#include <QVBoxLayout>

#include "addserverdialog.h"
#include "downloadtablemodel.h"
#include "ec/client.h"
#include "ec/codes.h"
#include "i18n.h"
#include "preferencespanel.h"
#include "progressbardelegate.h"
#include "searchpanel.h"
#include "servertablemodel.h"
#include "sharedfilemodel.h"
#include "sourcetablemodel.h"
#include "speedgraph.h"

namespace amule {
namespace {

// First-run fallback (ADR-0006): read the default Host/Port from aMule's own
// remote control config, ~/.aMule/remote.conf [EC]. Returns empty/0 if absent.
struct RemoteConfEc {
    QString host;
    int port = 0;
};

RemoteConfEc readRemoteConfEc() {
    const QString path = QDir::homePath() + QStringLiteral("/.aMule/remote.conf");
    if (!QFileInfo::exists(path))
        return {};
    QSettings conf(path, QSettings::IniFormat);
    RemoteConfEc ec;
    ec.host = conf.value(QStringLiteral("EC/Host")).toString().trimmed();
    ec.port = conf.value(QStringLiteral("EC/Port")).toInt();
    return ec;
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    buildMenuBar();
    wireWorker();
    loadSettings();
    onStatusChanged(ConnStatus::Disconnected, QString());

    // Restore the persisted speed history (after the disconnected-state reset,
    // which clears the graph).
    const QString dataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    speedHistoryPath_ = dataDir + QStringLiteral("/speed_history.dat");
    speedGraph_->load(speedHistoryPath_);

    // Restore the window geometry and dock/toolbar layout (e.g. whether the Log
    // dock was left visible) from the last session.
    QSettings settings;
    if (const QByteArray geometry = settings.value(QStringLiteral("geometry")).toByteArray();
        !geometry.isEmpty())
        restoreGeometry(geometry);
    if (const QByteArray state = settings.value(QStringLiteral("windowState")).toByteArray();
        !state.isEmpty())
        restoreState(state);

    // Welcome banner on the LED panel until the first connection.
    speedGraph_->showMessage(
        tr("Amule-Remote Qt %1").arg(QStringLiteral(APP_VERSION)));
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings;
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    settings.setValue(QStringLiteral("windowState"), saveState());
    if (!speedHistoryPath_.isEmpty())
        speedGraph_->save(speedHistoryPath_);
    QMainWindow::closeEvent(event);
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    // Connection bar.
    auto* bar = new QHBoxLayout;
    hostEdit_ = new QLineEdit;
    hostEdit_->setPlaceholderText(tr("host"));
    portSpin_ = new QSpinBox;
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(4712);
    passEdit_ = new QLineEdit;
    passEdit_->setPlaceholderText(tr("password"));
    passEdit_->setEchoMode(QLineEdit::Password);
    rememberCheck_ = new QCheckBox(tr("Remember"));
    connectBtn_ = new QPushButton(tr("Connect"));
    disconnectBtn_ = new QPushButton(tr("Disconnect"));

    bar->addWidget(new QLabel(tr("Host:")));
    bar->addWidget(hostEdit_, 2);
    bar->addWidget(new QLabel(tr("Port:")));
    bar->addWidget(portSpin_);
    bar->addWidget(new QLabel(tr("Pass:")));
    bar->addWidget(passEdit_, 1);
    bar->addWidget(rememberCheck_);
    bar->addWidget(connectBtn_);
    bar->addWidget(disconnectBtn_);
    layout->addLayout(bar);

    // Speed graph.
    speedGraph_ = new SpeedGraph;
    layout->addWidget(speedGraph_);

    // Download table, behind a sorting proxy so columns sort by value (numeric
    // sort keys come from the model's Qt::UserRole).
    model_ = new DownloadTableModel(this);
    downloadProxy_ = new QSortFilterProxyModel(this);
    downloadProxy_->setSourceModel(model_);
    downloadProxy_->setSortRole(Qt::UserRole);
    table_ = new QTableView;
    table_->setModel(downloadProxy_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Extended selection: click = single, Ctrl+click = toggle, Shift+click = range.
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setSortingEnabled(true);
    table_->verticalHeader()->setVisible(false);
    table_->setItemDelegateForColumn(DownloadTableModel::Progress,
                                     new ProgressBarDelegate(this));
    table_->setContextMenuPolicy(Qt::CustomContextMenu);
    table_->setShowGrid(false);
    auto* header = table_->horizontalHeader();
    header->setSectionResizeMode(DownloadTableModel::PartNum, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(DownloadTableModel::Name, QHeaderView::Stretch);
    for (int c = DownloadTableModel::Status; c < DownloadTableModel::ColumnCount; ++c)
        header->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    // Give the progress bar a stable, readable width.
    header->setSectionResizeMode(DownloadTableModel::Progress, QHeaderView::Interactive);
    table_->setColumnWidth(DownloadTableModel::Progress, 140);

    const auto configureTable = [](QTableView* view, int stretchColumn) {
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view->setAlternatingRowColors(true);
        view->setShowGrid(false);
        view->verticalHeader()->setVisible(false);
        view->horizontalHeader()->setSectionResizeMode(stretchColumn,
                                                        QHeaderView::Stretch);
    };

    auto* tabs = new QTabWidget;

    // Transfers tab: the download queue on top, sources for the selected
    // download below.
    sourceModel_ = new SourceTableModel(this);
    auto* sourceTable = new QTableView;
    sourceTable->setModel(sourceModel_);
    configureTable(sourceTable, SourceTableModel::Client);

    auto* split = new QSplitter(Qt::Vertical);
    split->addWidget(table_);
    split->addWidget(sourceTable);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);

    // Transfer-wide actions (moved here from the former toolbar).
    auto* transfersTab = new QWidget;
    auto* transfersLayout = new QVBoxLayout(transfersTab);
    transfersLayout->setContentsMargins(0, 0, 0, 0);
    auto* transfersButtons = new QHBoxLayout;
    auto* addLinkBtn = new QPushButton(QIcon(QStringLiteral(":/icons/add.svg")),
                                       tr("Add ed2k link…"));
    auto* clearBtn = new QPushButton(QIcon(QStringLiteral(":/icons/clear.svg")),
                                     tr("Clear completed"));
    transfersButtons->addWidget(addLinkBtn);
    transfersButtons->addWidget(clearBtn);
    transfersButtons->addStretch(1);
    transfersLayout->addLayout(transfersButtons);
    transfersLayout->addWidget(split);
    tabs->addTab(transfersTab, tr("Transfers"));

    connect(addLinkBtn, &QPushButton::clicked, this, &MainWindow::onAddLink);
    connect(clearBtn, &QPushButton::clicked, this, [this] {
        QMetaObject::invokeMethod(worker(), "clearCompleted", Qt::QueuedConnection);
    });
    connectedWidgets_ << addLinkBtn << clearBtn;

    // Search tab.
    searchPanel_ = new SearchPanel;
    tabs->addTab(searchPanel_, tr("Search"));

    // Servers tab.
    serverModel_ = new ServerTableModel(this);
    auto* serverProxy = new QSortFilterProxyModel(this);
    serverProxy->setSourceModel(serverModel_);
    serverProxy->setSortRole(Qt::UserRole);
    serverTable_ = new QTableView;
    serverTable_->setModel(serverProxy);
    configureTable(serverTable_, ServerTableModel::Name);
    serverTable_->setSortingEnabled(true);
    serverTable_->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* serversTab = new QWidget;
    auto* serversLayout = new QVBoxLayout(serversTab);
    serversLayout->setContentsMargins(0, 0, 0, 0);

    // Network/server connection controls (moved here from the toolbar). Helper
    // builds a command button bound to a worker slot, enabled only when
    // connected.
    const auto cmdButton = [this](const QString& text, const QString& iconName,
                                  const char* slot) {
        auto* button = new QPushButton(
            QIcon(QStringLiteral(":/icons/%1.svg").arg(iconName)), text);
        connect(button, &QPushButton::clicked, this, [this, slot] {
            QMetaObject::invokeMethod(worker(), slot, Qt::QueuedConnection);
        });
        connectedWidgets_ << button;
        return button;
    };

    auto* networkRow = new QHBoxLayout;
    networkRow->addWidget(cmdButton(tr("Connect networks"),
                                    QStringLiteral("connect"), "connectNetworks"));
    networkRow->addWidget(cmdButton(tr("Disconnect networks"),
                                    QStringLiteral("disconnect"), "disconnectNetworks"));
    networkRow->addWidget(cmdButton(tr("Start Kad"),
                                    QStringLiteral("kad-start"), "startKad"));
    networkRow->addWidget(cmdButton(tr("Stop Kad"),
                                    QStringLiteral("kad-stop"), "stopKad"));
    networkRow->addStretch(1);
    serversLayout->addLayout(networkRow);

    auto* serverRow = new QHBoxLayout;
    serverRow->addWidget(cmdButton(tr("Connect server"),
                                   QStringLiteral("server"), "serverConnectAny"));
    serverRow->addWidget(cmdButton(tr("Disconnect server"),
                                   QStringLiteral("server-off"), "serverDisconnect"));
    auto* addServerBtn = new QPushButton(QIcon(QStringLiteral(":/icons/add.svg")),
                                         tr("Add server…"));
    auto* updateServersBtn = new QPushButton(
        QIcon(QStringLiteral(":/icons/refresh.svg")), tr("Update from URL…"));
    serverRow->addWidget(addServerBtn);
    serverRow->addWidget(updateServersBtn);
    serverRow->addStretch(1);
    serversLayout->addLayout(serverRow);

    serversLayout->addWidget(serverTable_);
    tabs->addTab(serversTab, tr("Servers"));

    connect(addServerBtn, &QPushButton::clicked, this, &MainWindow::onAddServer);
    connect(updateServersBtn, &QPushButton::clicked, this, &MainWindow::onUpdateServers);
    connectedWidgets_ << addServerBtn << updateServersBtn;

    // Shared tab.
    sharedModel_ = new SharedFileModel(this);
    auto* sharedProxy = new QSortFilterProxyModel(this);
    sharedProxy->setSourceModel(sharedModel_);
    sharedProxy->setSortRole(Qt::UserRole);
    auto* sharedTable = new QTableView;
    sharedTable->setModel(sharedProxy);
    configureTable(sharedTable, SharedFileModel::Name);
    sharedTable->setSortingEnabled(true);

    auto* sharedTab = new QWidget;
    auto* sharedLayout = new QVBoxLayout(sharedTab);
    sharedLayout->setContentsMargins(0, 0, 0, 0);
    auto* sharedButtons = new QHBoxLayout;
    auto* reloadSharedBtn = new QPushButton(QIcon(QStringLiteral(":/icons/refresh.svg")),
                                            tr("Refresh"));
    sharedButtons->addWidget(reloadSharedBtn);
    sharedButtons->addStretch(1);
    sharedLayout->addLayout(sharedButtons);
    sharedLayout->addWidget(sharedTable);
    tabs->addTab(sharedTab, tr("Shared"));

    connect(reloadSharedBtn, &QPushButton::clicked, this, [this] {
        QMetaObject::invokeMethod(worker(), "reloadShared", Qt::QueuedConnection);
    });
    connectedWidgets_ << reloadSharedBtn;

    // Preferences tab (Connection + Directories subtabs).
    preferencesPanel_ = new PreferencesPanel;
    tabs->addTab(preferencesPanel_, tr("Preferences"));
    connectedWidgets_ << preferencesPanel_;

    layout->addWidget(tabs, 1);

    setCentralWidget(central);

    // Activity log dock. The object name lets QMainWindow::saveState persist
    // its visibility/position across runs.
    auto* dock = new QDockWidget(tr("Log"), this);
    dock->setObjectName(QStringLiteral("logDock"));
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
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::onTransferSelectionChanged);
    connect(serverTable_, &QTableView::customContextMenuRequested, this,
            &MainWindow::onServerContextMenu);
}

void MainWindow::onAddLink() {
    bool ok = false;
    const QString link = QInputDialog::getText(
        this, tr("Add ed2k link"), tr("ed2k:// link:"),
        QLineEdit::Normal, QString(), &ok);
    if (ok && !link.trimmed().isEmpty())
        QMetaObject::invokeMethod(worker(), "addLink", Qt::QueuedConnection,
                                  Q_ARG(QString, link.trimmed()));
}

void MainWindow::buildMenuBar() {
    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    auto* langMenu = viewMenu->addMenu(tr("Language"));
    auto* group = new QActionGroup(this);

    const QString current = i18n::currentSetting();
    const auto addLang = [&](const QString& label, const QString& code) {
        QAction* action = langMenu->addAction(label);
        action->setCheckable(true);
        action->setChecked(current == code);
        group->addAction(action);
        connect(action, &QAction::triggered, this, [this, code] {
            i18n::setSetting(code);
            QMessageBox::information(
                this, tr("Language"),
                tr("The language change takes effect when you restart the application."));
        });
    };

    addLang(tr("System default"), QString());
    langMenu->addSeparator();
    for (const i18n::Language& lang : i18n::languages())
        addLang(lang.nativeName, lang.code);
}

void MainWindow::wireWorker() {
    EcWorker* w = worker();
    // Queued automatically: the worker lives on another thread.
    connect(w, &EcWorker::statusChanged, this, &MainWindow::onStatusChanged);
    connect(w, &EcWorker::statsUpdated, this, &MainWindow::onStats);
    connect(w, &EcWorker::connStateUpdated, this, &MainWindow::onConnState);
    connect(w, &EcWorker::prefsUpdated, preferencesPanel_, &PreferencesPanel::setPrefs);
    connect(preferencesPanel_, &PreferencesPanel::applyRequested, this,
            [this](DaemonPrefs prefs) {
                QMetaObject::invokeMethod(worker(), "applyPrefs", Qt::QueuedConnection,
                                          Q_ARG(amule::DaemonPrefs, prefs));
            });
    connect(preferencesPanel_, &PreferencesPanel::reloadRequested, this, [this] {
        QMetaObject::invokeMethod(worker(), "fetchPrefs", Qt::QueuedConnection);
    });
    connect(w, &EcWorker::logMessage, this, &MainWindow::onLog);
    connect(w, &EcWorker::downloadsUpdated, model_, &DownloadTableModel::setDownloads);

    // Search panel <-> worker.
    connect(w, &EcWorker::searchResultsUpdated, searchPanel_, &SearchPanel::setResults);
    connect(w, &EcWorker::searchProgressChanged, searchPanel_, &SearchPanel::setProgress);
    connect(w, &EcWorker::searchingChanged, searchPanel_, &SearchPanel::setSearching);
    connect(searchPanel_, &SearchPanel::searchRequested, this, [this](SearchParams params) {
        QMetaObject::invokeMethod(worker(), "startSearch", Qt::QueuedConnection,
                                  Q_ARG(amule::SearchParams, params));
    });
    connect(searchPanel_, &SearchPanel::stopRequested, this, [this] {
        QMetaObject::invokeMethod(worker(), "stopSearch", Qt::QueuedConnection);
    });
    connect(searchPanel_, &SearchPanel::downloadRequested, this, [this](Hash16 hash) {
        QMetaObject::invokeMethod(worker(), "downloadResult", Qt::QueuedConnection,
                                  Q_ARG(amule::Hash16, hash));
    });

    // Servers, shared files and sources.
    connect(w, &EcWorker::serversUpdated, serverModel_, &ServerTableModel::setServers);
    connect(w, &EcWorker::sharedFilesUpdated, sharedModel_, &SharedFileModel::setFiles);
    connect(w, &EcWorker::sourcesUpdated, sourceModel_, &SourceTableModel::setSources);
}

void MainWindow::onTransferSelectionChanged() {
    const QModelIndex index = table_->selectionModel()->currentIndex();
    const int row = index.isValid() ? downloadProxy_->mapToSource(index).row() : -1;
    sourceModel_->setFileEcid(row >= 0 ? model_->ecidAt(row) : 0);
}

void MainWindow::onServerContextMenu(const QPoint& pos) {
    const QModelIndex index = serverTable_->indexAt(pos);
    if (!index.isValid())
        return;
    auto* proxy = qobject_cast<QSortFilterProxyModel*>(serverTable_->model());
    const int row = proxy ? proxy->mapToSource(index).row() : index.row();
    const ServerIp ip = serverModel_->ipAt(row);
    const quint16 port = serverModel_->portAt(row);

    QMenu menu(this);
    QAction* connectAct = menu.addAction(tr("Connect"));
    QAction* removeAct = menu.addAction(tr("Remove"));

    QAction* chosen = menu.exec(serverTable_->viewport()->mapToGlobal(pos));
    if (chosen == connectAct)
        QMetaObject::invokeMethod(worker(), "serverConnect", Qt::QueuedConnection,
                                  Q_ARG(amule::ServerIp, ip), Q_ARG(quint16, port));
    else if (chosen == removeAct)
        QMetaObject::invokeMethod(worker(), "serverRemove", Qt::QueuedConnection,
                                  Q_ARG(amule::ServerIp, ip), Q_ARG(quint16, port));
}

void MainWindow::onConnectClicked() {
    const QString host = hostEdit_->text().trimmed();
    if (host.isEmpty()) {
        onLog(tr("enter a host first"));
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

    // Act on every selected download; fall back to the row under the cursor.
    QList<Hash16> hashes;
    const QModelIndexList selected = table_->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        hashes.append(model_->hashAt(downloadProxy_->mapToSource(index).row()));
    } else {
        for (const QModelIndex& i : selected)
            hashes.append(model_->hashAt(downloadProxy_->mapToSource(i).row()));
    }

    QMenu menu(this);
    QAction* pause = menu.addAction(tr("Pause"));
    QAction* resume = menu.addAction(tr("Resume"));
    QAction* stop = menu.addAction(tr("Stop"));

    QMenu* priorityMenu = menu.addMenu(tr("Priority"));
    QAction* prioLow = priorityMenu->addAction(tr("Low"));
    QAction* prioNormal = priorityMenu->addAction(tr("Normal"));
    QAction* prioHigh = priorityMenu->addAction(tr("High"));
    QAction* prioAuto = priorityMenu->addAction(tr("Auto"));

    menu.addSeparator();
    QAction* remove = menu.addAction(tr("Delete…"));

    QAction* chosen = menu.exec(table_->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    // Priority submenu: send setPriority for each and return.
    quint8 priority = 0;
    bool isPriority = true;
    if (chosen == prioLow)
        priority = ec::prio::LOW;
    else if (chosen == prioNormal)
        priority = ec::prio::NORMAL;
    else if (chosen == prioHigh)
        priority = ec::prio::HIGH;
    else if (chosen == prioAuto)
        priority = ec::prio::AUTO;
    else
        isPriority = false;
    if (isPriority) {
        for (const Hash16& hash : hashes)
            QMetaObject::invokeMethod(worker(), "setPriority", Qt::QueuedConnection,
                                      Q_ARG(amule::Hash16, hash), Q_ARG(quint8, priority));
        return;
    }

    const char* slot = nullptr;
    if (chosen == pause)
        slot = "pause";
    else if (chosen == resume)
        slot = "resume";
    else if (chosen == stop)
        slot = "stop";
    else if (chosen == remove) {
        if (QMessageBox::question(
                this, tr("Delete download"),
                tr("Delete %1 download(s) from the queue?").arg(hashes.size())) !=
            QMessageBox::Yes)
            return;
        slot = "remove";
    }
    if (slot)
        for (const Hash16& hash : hashes)
            QMetaObject::invokeMethod(worker(), slot, Qt::QueuedConnection,
                                      Q_ARG(amule::Hash16, hash));
}

void MainWindow::onStatusChanged(ConnStatus status, const QString& detail) {
    bool connected = false;
    bool connecting = false;
    QString color;
    switch (status) {
    case ConnStatus::Disconnected:
        statusLabel_->setText(tr("Disconnected"));
        break;
    case ConnStatus::Connecting:
        statusLabel_->setText(tr("Connecting to %1…").arg(detail));
        color = QStringLiteral("#d9a441");
        connecting = true;
        break;
    case ConnStatus::Connected:
        statusLabel_->setText(tr("Connected — daemon %1").arg(detail));
        color = QStringLiteral("#4cc06a");
        connected = true;
        speedGraph_->dismissMessage(); // scroll the welcome banner away
        break;
    case ConnStatus::Error:
        statusLabel_->setText(tr("Error: %1").arg(detail));
        color = QStringLiteral("#d9534f");
        break;
    }
    statusLabel_->setStyleSheet(
        color.isEmpty() ? QString()
                        : QStringLiteral("color:%1; font-weight:bold;").arg(color));
    connectBtn_->setEnabled(!connected && !connecting);
    disconnectBtn_->setEnabled(connected);
    for (QWidget* widget : connectedWidgets_)
        widget->setEnabled(connected);
    if (!connected) {
        netLabel_->clear();
        statsLabel_->clear();
        speedGraph_->clear();
    }
}

void MainWindow::onStats(Stats stats) {
    speedGraph_->addSample(stats.dlSpeed, stats.ulSpeed);
    statsLabel_->setText(tr("↓ %1   ↑ %2   ed2k %3 users%4")
                             .arg(humanRate(stats.dlSpeed), humanRate(stats.ulSpeed),
                                  humanCount(stats.ed2kUsers),
                                  stats.isHighId() ? tr("   HighID")
                                                   : QString()));
}

void MainWindow::onConnState(ConnState conn) {
    const QString ed2k = conn.ed2kConnected
                             ? tr("ed2k: %1").arg(conn.ed2kServer)
                             : tr("ed2k: off");
    const QString kad =
        conn.kadConnected ? tr("Kad: on") : tr("Kad: off");
    netLabel_->setText(QStringLiteral("%1   |   %2").arg(ed2k, kad));
}

void MainWindow::onAddServer() {
    AddServerDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted || dialog.address().isEmpty())
        return;
    QMetaObject::invokeMethod(worker(), "serverAdd", Qt::QueuedConnection,
                              Q_ARG(QString, dialog.serverName()),
                              Q_ARG(QString, dialog.address()));
}

void MainWindow::onUpdateServers() {
    bool ok = false;
    const QString url = QInputDialog::getText(
        this, tr("Update server list"),
        tr("server.met URL:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !url.trimmed().isEmpty())
        QMetaObject::invokeMethod(worker(), "serverUpdateFromUrl",
                                  Qt::QueuedConnection, Q_ARG(QString, url.trimmed()));
}

void MainWindow::onLog(const QString& message) {
    log_->appendPlainText(message);
}

void MainWindow::loadSettings() {
    QSettings settings;
    QString host = settings.value(QStringLiteral("host")).toString();
    int port = settings.value(QStringLiteral("port")).toInt();

    if (host.isEmpty()) {
        // First run: fall back to aMule's remote.conf, then built-in defaults.
        const RemoteConfEc ec = readRemoteConfEc();
        host = ec.host.isEmpty() ? QStringLiteral("127.0.0.1") : ec.host;
        if (port == 0)
            port = ec.port > 0 ? ec.port : 4712;
    }
    if (port == 0)
        port = 4712;

    hostEdit_->setText(host);
    portSpin_->setValue(port);

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
