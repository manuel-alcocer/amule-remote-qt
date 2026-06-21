#include "preferencespanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace amule {

PreferencesPanel::PreferencesPanel(QWidget* parent) : QWidget(parent) {
    auto* subtabs = new QTabWidget;
    subtabs->addTab(buildGeneralTab(), QStringLiteral("General"));
    subtabs->addTab(buildConnectionTab(), QStringLiteral("Connection"));
    subtabs->addTab(buildFilesTab(), QStringLiteral("Files"));
    subtabs->addTab(buildDirectoriesTab(), QStringLiteral("Directories"));
    subtabs->addTab(buildServersTab(), QStringLiteral("Servers"));
    subtabs->addTab(buildSecurityTab(), QStringLiteral("Security"));
    subtabs->addTab(buildMessagesTab(), QStringLiteral("Messages"));
    subtabs->addTab(buildRemoteTab(), QStringLiteral("Remote"));
    subtabs->addTab(buildAdvancedTab(), QStringLiteral("Advanced"));

    auto* reloadBtn = new QPushButton(QStringLiteral("Reload"));
    auto* applyBtn = new QPushButton(QStringLiteral("Apply"));
    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    buttons->addWidget(reloadBtn);
    buttons->addWidget(applyBtn);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(subtabs, 1);
    layout->addLayout(buttons);

    connect(applyBtn, &QPushButton::clicked, this,
            [this] { emit applyRequested(collect()); });
    connect(reloadBtn, &QPushButton::clicked, this,
            [this] { emit reloadRequested(); });
}

// --- Binding helpers -------------------------------------------------------

QCheckBox* PreferencesPanel::boolBox(const QString& text, bool DaemonPrefs::* field) {
    auto* box = new QCheckBox(text);
    bools_.append({box, field});
    return box;
}

QSpinBox* PreferencesPanel::intBox(int min, int max, quint64 DaemonPrefs::* field,
                                   const QString& suffix, bool unlimitedAtZero) {
    auto* box = new QSpinBox;
    box->setRange(min, max);
    if (!suffix.isEmpty())
        box->setSuffix(suffix);
    if (unlimitedAtZero)
        box->setSpecialValueText(QStringLiteral("Unlimited"));
    ints_.append({box, field});
    return box;
}

QLineEdit* PreferencesPanel::strEdit(QString DaemonPrefs::* field) {
    auto* edit = new QLineEdit;
    strings_.append({edit, field});
    return edit;
}

// --- Subtabs ---------------------------------------------------------------

QWidget* PreferencesPanel::buildGeneralTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(QStringLiteral("Nick:"), strEdit(&DaemonPrefs::nick));
    form->addRow(boolBox(QStringLiteral("Check for new version at startup"),
                         &DaemonPrefs::checkNewVersion));
    form->addRow(boolBox(QStringLiteral("Enable online signature"),
                         &DaemonPrefs::onlineSignatureEnabled));
    return w;
}

QWidget* PreferencesPanel::buildConnectionTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(QStringLiteral("Max download:"),
                 intBox(0, 1'000'000, &DaemonPrefs::maxDownload, QStringLiteral(" kB/s"), true));
    form->addRow(QStringLiteral("Max upload:"),
                 intBox(0, 1'000'000, &DaemonPrefs::maxUpload, QStringLiteral(" kB/s"), true));
    form->addRow(QStringLiteral("Upload slot rate:"),
                 intBox(0, 1'000'000, &DaemonPrefs::slotAllocation, QStringLiteral(" kB/s")));
    form->addRow(QStringLiteral("Download capacity:"),
                 intBox(0, 1'000'000, &DaemonPrefs::downloadCapacity, QStringLiteral(" kB/s")));
    form->addRow(QStringLiteral("Upload capacity:"),
                 intBox(0, 1'000'000, &DaemonPrefs::uploadCapacity, QStringLiteral(" kB/s")));
    form->addRow(QStringLiteral("TCP port:"), intBox(0, 65535, &DaemonPrefs::tcpPort));
    form->addRow(QStringLiteral("UDP port:"), intBox(0, 65535, &DaemonPrefs::udpPort));
    form->addRow(boolBox(QStringLiteral("Disable UDP port"), &DaemonPrefs::udpDisable));
    form->addRow(QStringLiteral("Max sources/file:"),
                 intBox(0, 100'000, &DaemonPrefs::maxSources));
    form->addRow(QStringLiteral("Max connections:"),
                 intBox(0, 100'000, &DaemonPrefs::maxConnections));
    form->addRow(boolBox(QStringLiteral("ED2K network"), &DaemonPrefs::networkEd2k));
    form->addRow(boolBox(QStringLiteral("Kad network"), &DaemonPrefs::networkKad));
    form->addRow(boolBox(QStringLiteral("Auto-connect on start"), &DaemonPrefs::autoconnect));
    form->addRow(boolBox(QStringLiteral("Reconnect on loss"), &DaemonPrefs::reconnect));
    return w;
}

QWidget* PreferencesPanel::buildFilesTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);
    box->addWidget(boolBox(QStringLiteral("Intelligent Corruption Handling (ICH)"),
                           &DaemonPrefs::ichEnabled));
    box->addWidget(boolBox(QStringLiteral("Advanced ICH trusts every hash"),
                           &DaemonPrefs::aichTrust));
    box->addWidget(boolBox(QStringLiteral("Add files to download in pause mode"),
                           &DaemonPrefs::addNewPaused));
    box->addWidget(boolBox(QStringLiteral("Add files to download with auto priority"),
                           &DaemonPrefs::addNewAutoDlPrio));
    box->addWidget(boolBox(QStringLiteral("Try to download first and last chunks first"),
                           &DaemonPrefs::previewPrio));
    box->addWidget(boolBox(QStringLiteral("Start next paused file when a file completes"),
                           &DaemonPrefs::startNextPaused));
    box->addWidget(boolBox(QStringLiteral("...from the same category"),
                           &DaemonPrefs::resumeSameCategory));
    box->addWidget(boolBox(QStringLiteral("Preallocate disk space for new files"),
                           &DaemonPrefs::allocFullSize));
    box->addWidget(boolBox(QStringLiteral("Upload full chunks"),
                           &DaemonPrefs::ulFullChunks));
    box->addWidget(boolBox(QStringLiteral("Extract metadata"),
                           &DaemonPrefs::extractMetadata));
    box->addWidget(boolBox(QStringLiteral("Save 10 sources on rare files (< 20 sources)"),
                           &DaemonPrefs::saveSources));

    auto* freeRow = new QHBoxLayout;
    freeRow->addWidget(boolBox(QStringLiteral("Stop downloads when free disk space reaches"),
                               &DaemonPrefs::checkFreeSpace));
    freeRow->addWidget(intBox(0, 1'000'000, &DaemonPrefs::minFreeSpaceMB, QStringLiteral(" MB")));
    freeRow->addStretch(1);
    box->addLayout(freeRow);

    box->addWidget(boolBox(QStringLiteral("Add new shared files with auto priority"),
                           &DaemonPrefs::addNewAutoUlPrio));
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildDirectoriesTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(QStringLiteral("Incoming:"), strEdit(&DaemonPrefs::incomingDir));
    form->addRow(QStringLiteral("Temp:"), strEdit(&DaemonPrefs::tempDir));
    form->addRow(boolBox(QStringLiteral("Share hidden files"), &DaemonPrefs::shareHidden));
    form->addRow(boolBox(QStringLiteral("Auto-rescan shared folders"),
                         &DaemonPrefs::autoRescan));
    return w;
}

QWidget* PreferencesPanel::buildServersTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);

    auto* deadRow = new QHBoxLayout;
    deadRow->addWidget(boolBox(QStringLiteral("Remove dead server after"),
                               &DaemonPrefs::removeDeadServers));
    deadRow->addWidget(intBox(0, 1000, &DaemonPrefs::deadServerRetries));
    deadRow->addWidget(new QLabel(QStringLiteral("retries")));
    deadRow->addStretch(1);
    box->addLayout(deadRow);

    box->addWidget(boolBox(QStringLiteral("Auto-update server list at startup"),
                           &DaemonPrefs::serverAutoUpdate));
    box->addWidget(boolBox(QStringLiteral("Update server list when connecting to a server"),
                           &DaemonPrefs::updateFromServer));
    box->addWidget(boolBox(QStringLiteral("Update server list when a client connects"),
                           &DaemonPrefs::updateFromClient));
    box->addWidget(boolBox(QStringLiteral("Use priority system"),
                           &DaemonPrefs::useScoreSystem));
    box->addWidget(boolBox(QStringLiteral("Use smart LowID check on connect"),
                           &DaemonPrefs::smartIdCheck));
    box->addWidget(boolBox(QStringLiteral("Safe connect"), &DaemonPrefs::safeServerConnect));
    box->addWidget(boolBox(QStringLiteral("Autoconnect to servers in static list only"),
                           &DaemonPrefs::autoConnectStaticOnly));
    box->addWidget(boolBox(QStringLiteral("Set manually added servers to High Priority"),
                           &DaemonPrefs::manualHighPrio));

    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Server list URL:"), strEdit(&DaemonPrefs::serverListUrl));
    box->addLayout(form);
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildSecurityTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);

    box->addWidget(boolBox(QStringLiteral("Use Secure User Identification"),
                           &DaemonPrefs::useSecIdent));

    auto* obf = new QGroupBox(QStringLiteral("Protocol obfuscation"));
    auto* obfBox = new QVBoxLayout(obf);
    obfBox->addWidget(boolBox(QStringLiteral("Use obfuscation for outgoing connections"),
                              &DaemonPrefs::obfuscationRequested));
    obfBox->addWidget(boolBox(QStringLiteral("Accept only obfuscated connections"),
                              &DaemonPrefs::obfuscationRequired));
    box->addWidget(obf);

    auto* sharesForm = new QFormLayout;
    canSeeShares_ = new QComboBox;
    canSeeShares_->addItem(QStringLiteral("Everybody"));
    canSeeShares_->addItem(QStringLiteral("Friends"));
    canSeeShares_->addItem(QStringLiteral("No one"));
    sharesForm->addRow(QStringLiteral("Who can see my shared files:"), canSeeShares_);
    box->addLayout(sharesForm);

    auto* ipf = new QGroupBox(QStringLiteral("IP filtering"));
    auto* ipfBox = new QVBoxLayout(ipf);
    ipfBox->addWidget(boolBox(QStringLiteral("Filter clients"), &DaemonPrefs::filterClients));
    ipfBox->addWidget(boolBox(QStringLiteral("Filter servers"), &DaemonPrefs::filterServers));
    ipfBox->addWidget(boolBox(QStringLiteral("Auto-update ipfilter at startup"),
                              &DaemonPrefs::ipfilterAutoUpdate));
    auto* ipfForm = new QFormLayout;
    ipfForm->addRow(QStringLiteral("URL:"), strEdit(&DaemonPrefs::ipfilterUrl));
    ipfForm->addRow(QStringLiteral("Filtering level:"),
                    intBox(0, 255, &DaemonPrefs::ipfilterLevel));
    ipfBox->addLayout(ipfForm);
    ipfBox->addWidget(boolBox(QStringLiteral("Always filter LAN IPs"), &DaemonPrefs::filterLan));
    box->addWidget(ipf);
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildMessagesTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);
    box->addWidget(boolBox(QStringLiteral("Filter incoming messages (except current chat)"),
                           &DaemonPrefs::msgFilterEnabled));
    box->addWidget(boolBox(QStringLiteral("Filter all messages"), &DaemonPrefs::msgFilterAll));
    box->addWidget(boolBox(QStringLiteral("Filter messages from people not on your friend list"),
                           &DaemonPrefs::msgFilterFriends));
    box->addWidget(boolBox(QStringLiteral("Filter messages from unknown clients"),
                           &DaemonPrefs::msgFilterSecure));
    box->addWidget(boolBox(QStringLiteral("Filter messages containing keywords"),
                           &DaemonPrefs::msgFilterByKeyword));
    auto* form = new QFormLayout;
    form->addRow(QStringLiteral("Keywords (',' separated):"),
                 strEdit(&DaemonPrefs::msgKeywords));
    box->addLayout(form);
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildRemoteTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(boolBox(QStringLiteral("Run webserver on startup"),
                         &DaemonPrefs::webserverAutorun));
    form->addRow(QStringLiteral("Web server TCP port:"),
                 intBox(0, 65535, &DaemonPrefs::webserverPort));
    form->addRow(boolBox(QStringLiteral("Enable Low rights user (guest)"),
                         &DaemonPrefs::webserverGuest));
    form->addRow(boolBox(QStringLiteral("Enable Gzip compression"),
                         &DaemonPrefs::webserverGzip));
    form->addRow(QStringLiteral("Page refresh time (s):"),
                 intBox(0, 100'000, &DaemonPrefs::webserverRefresh));
    form->addRow(QStringLiteral("Web template:"), strEdit(&DaemonPrefs::webserverTemplate));
    return w;
}

QWidget* PreferencesPanel::buildAdvancedTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(QStringLiteral("Max new connections / 5 s:"),
                 intBox(0, 1000, &DaemonPrefs::maxConnPer5Sec));
    form->addRow(QStringLiteral("File buffer size (bytes):"),
                 intBox(0, 2'000'000'000, &DaemonPrefs::fileBufferSize));
    form->addRow(QStringLiteral("Upload queue size (clients):"),
                 intBox(0, 1'000'000, &DaemonPrefs::uploadQueueSize));
    form->addRow(QStringLiteral("Server connection refresh (s):"),
                 intBox(0, 1'000'000, &DaemonPrefs::serverKeepaliveTimeout));
    form->addRow(boolBox(QStringLiteral("Verbose logging"), &DaemonPrefs::verbose));
    form->addRow(QStringLiteral("Kad nodes URL:"), strEdit(&DaemonPrefs::kadNodesUrl));
    return w;
}

// --- Apply / reload --------------------------------------------------------

void PreferencesPanel::setPrefs(DaemonPrefs prefs) {
    current_ = prefs;
    for (const BoolBinding& b : bools_)
        b.widget->setChecked(prefs.*(b.field));
    for (const IntBinding& i : ints_)
        i.widget->setValue(static_cast<int>(prefs.*(i.field)));
    for (const StringBinding& s : strings_)
        s.widget->setText(prefs.*(s.field));
    if (canSeeShares_)
        canSeeShares_->setCurrentIndex(static_cast<int>(prefs.canSeeShares));
}

DaemonPrefs PreferencesPanel::collect() const {
    DaemonPrefs p = current_; // preserve loaded flag and read-only fields
    for (const BoolBinding& b : bools_)
        p.*(b.field) = b.widget->isChecked();
    for (const IntBinding& i : ints_)
        p.*(i.field) = static_cast<quint64>(i.widget->value());
    for (const StringBinding& s : strings_)
        p.*(s.field) = s.widget->text();
    if (canSeeShares_)
        p.canSeeShares = static_cast<quint64>(canSeeShares_->currentIndex());
    return p;
}

} // namespace amule
