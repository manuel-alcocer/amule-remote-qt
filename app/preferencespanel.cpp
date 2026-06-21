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
    subtabs->addTab(buildGeneralTab(), tr("General"));
    subtabs->addTab(buildConnectionTab(), tr("Connection"));
    subtabs->addTab(buildFilesTab(), tr("Files"));
    subtabs->addTab(buildDirectoriesTab(), tr("Directories"));
    subtabs->addTab(buildServersTab(), tr("Servers"));
    subtabs->addTab(buildSecurityTab(), tr("Security"));
    subtabs->addTab(buildMessagesTab(), tr("Messages"));
    subtabs->addTab(buildRemoteTab(), tr("Remote"));
    subtabs->addTab(buildAdvancedTab(), tr("Advanced"));

    auto* reloadBtn = new QPushButton(tr("Reload"));
    auto* applyBtn = new QPushButton(tr("Apply"));
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
        box->setSpecialValueText(tr("Unlimited"));
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
    form->addRow(tr("Nick:"), strEdit(&DaemonPrefs::nick));
    form->addRow(boolBox(tr("Check for new version at startup"),
                         &DaemonPrefs::checkNewVersion));
    form->addRow(boolBox(tr("Enable online signature"),
                         &DaemonPrefs::onlineSignatureEnabled));
    return w;
}

QWidget* PreferencesPanel::buildConnectionTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(tr("Max download:"),
                 intBox(0, 1'000'000, &DaemonPrefs::maxDownload, tr(" kB/s"), true));
    form->addRow(tr("Max upload:"),
                 intBox(0, 1'000'000, &DaemonPrefs::maxUpload, tr(" kB/s"), true));
    form->addRow(tr("Upload slot rate:"),
                 intBox(0, 1'000'000, &DaemonPrefs::slotAllocation, tr(" kB/s")));
    form->addRow(tr("Download capacity:"),
                 intBox(0, 1'000'000, &DaemonPrefs::downloadCapacity, tr(" kB/s")));
    form->addRow(tr("Upload capacity:"),
                 intBox(0, 1'000'000, &DaemonPrefs::uploadCapacity, tr(" kB/s")));
    form->addRow(tr("TCP port:"), intBox(0, 65535, &DaemonPrefs::tcpPort));
    form->addRow(tr("UDP port:"), intBox(0, 65535, &DaemonPrefs::udpPort));
    form->addRow(boolBox(tr("Disable UDP port"), &DaemonPrefs::udpDisable));
    form->addRow(tr("Max sources/file:"),
                 intBox(0, 100'000, &DaemonPrefs::maxSources));
    form->addRow(tr("Max connections:"),
                 intBox(0, 100'000, &DaemonPrefs::maxConnections));
    form->addRow(boolBox(tr("ED2K network"), &DaemonPrefs::networkEd2k));
    form->addRow(boolBox(tr("Kad network"), &DaemonPrefs::networkKad));
    form->addRow(boolBox(tr("Auto-connect on start"), &DaemonPrefs::autoconnect));
    form->addRow(boolBox(tr("Reconnect on loss"), &DaemonPrefs::reconnect));
    return w;
}

QWidget* PreferencesPanel::buildFilesTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);
    box->addWidget(boolBox(tr("Intelligent Corruption Handling (ICH)"),
                           &DaemonPrefs::ichEnabled));
    box->addWidget(boolBox(tr("Advanced ICH trusts every hash"),
                           &DaemonPrefs::aichTrust));
    box->addWidget(boolBox(tr("Add files to download in pause mode"),
                           &DaemonPrefs::addNewPaused));
    box->addWidget(boolBox(tr("Add files to download with auto priority"),
                           &DaemonPrefs::addNewAutoDlPrio));
    box->addWidget(boolBox(tr("Try to download first and last chunks first"),
                           &DaemonPrefs::previewPrio));
    box->addWidget(boolBox(tr("Start next paused file when a file completes"),
                           &DaemonPrefs::startNextPaused));
    box->addWidget(boolBox(tr("...from the same category"),
                           &DaemonPrefs::resumeSameCategory));
    box->addWidget(boolBox(tr("Preallocate disk space for new files"),
                           &DaemonPrefs::allocFullSize));
    box->addWidget(boolBox(tr("Upload full chunks"),
                           &DaemonPrefs::ulFullChunks));
    box->addWidget(boolBox(tr("Extract metadata"),
                           &DaemonPrefs::extractMetadata));
    box->addWidget(boolBox(tr("Save 10 sources on rare files (< 20 sources)"),
                           &DaemonPrefs::saveSources));

    auto* freeRow = new QHBoxLayout;
    freeRow->addWidget(boolBox(tr("Stop downloads when free disk space reaches"),
                               &DaemonPrefs::checkFreeSpace));
    freeRow->addWidget(intBox(0, 1'000'000, &DaemonPrefs::minFreeSpaceMB, tr(" MB")));
    freeRow->addStretch(1);
    box->addLayout(freeRow);

    box->addWidget(boolBox(tr("Add new shared files with auto priority"),
                           &DaemonPrefs::addNewAutoUlPrio));
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildDirectoriesTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(tr("Incoming:"), strEdit(&DaemonPrefs::incomingDir));
    form->addRow(tr("Temp:"), strEdit(&DaemonPrefs::tempDir));
    form->addRow(boolBox(tr("Share hidden files"), &DaemonPrefs::shareHidden));
    form->addRow(boolBox(tr("Auto-rescan shared folders"),
                         &DaemonPrefs::autoRescan));
    return w;
}

QWidget* PreferencesPanel::buildServersTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);

    auto* deadRow = new QHBoxLayout;
    deadRow->addWidget(boolBox(tr("Remove dead server after"),
                               &DaemonPrefs::removeDeadServers));
    deadRow->addWidget(intBox(0, 1000, &DaemonPrefs::deadServerRetries));
    deadRow->addWidget(new QLabel(tr("retries")));
    deadRow->addStretch(1);
    box->addLayout(deadRow);

    box->addWidget(boolBox(tr("Auto-update server list at startup"),
                           &DaemonPrefs::serverAutoUpdate));
    box->addWidget(boolBox(tr("Update server list when connecting to a server"),
                           &DaemonPrefs::updateFromServer));
    box->addWidget(boolBox(tr("Update server list when a client connects"),
                           &DaemonPrefs::updateFromClient));
    box->addWidget(boolBox(tr("Use priority system"),
                           &DaemonPrefs::useScoreSystem));
    box->addWidget(boolBox(tr("Use smart LowID check on connect"),
                           &DaemonPrefs::smartIdCheck));
    box->addWidget(boolBox(tr("Safe connect"), &DaemonPrefs::safeServerConnect));
    box->addWidget(boolBox(tr("Autoconnect to servers in static list only"),
                           &DaemonPrefs::autoConnectStaticOnly));
    box->addWidget(boolBox(tr("Set manually added servers to High Priority"),
                           &DaemonPrefs::manualHighPrio));

    auto* form = new QFormLayout;
    form->addRow(tr("Server list URL:"), strEdit(&DaemonPrefs::serverListUrl));
    box->addLayout(form);
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildSecurityTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);

    box->addWidget(boolBox(tr("Use Secure User Identification"),
                           &DaemonPrefs::useSecIdent));

    auto* obf = new QGroupBox(tr("Protocol obfuscation"));
    auto* obfBox = new QVBoxLayout(obf);
    obfBox->addWidget(boolBox(tr("Use obfuscation for outgoing connections"),
                              &DaemonPrefs::obfuscationRequested));
    obfBox->addWidget(boolBox(tr("Accept only obfuscated connections"),
                              &DaemonPrefs::obfuscationRequired));
    box->addWidget(obf);

    auto* sharesForm = new QFormLayout;
    canSeeShares_ = new QComboBox;
    canSeeShares_->addItem(tr("Everybody"));
    canSeeShares_->addItem(tr("Friends"));
    canSeeShares_->addItem(tr("No one"));
    sharesForm->addRow(tr("Who can see my shared files:"), canSeeShares_);
    box->addLayout(sharesForm);

    auto* ipf = new QGroupBox(tr("IP filtering"));
    auto* ipfBox = new QVBoxLayout(ipf);
    ipfBox->addWidget(boolBox(tr("Filter clients"), &DaemonPrefs::filterClients));
    ipfBox->addWidget(boolBox(tr("Filter servers"), &DaemonPrefs::filterServers));
    ipfBox->addWidget(boolBox(tr("Auto-update ipfilter at startup"),
                              &DaemonPrefs::ipfilterAutoUpdate));
    auto* ipfForm = new QFormLayout;
    ipfForm->addRow(tr("URL:"), strEdit(&DaemonPrefs::ipfilterUrl));
    ipfForm->addRow(tr("Filtering level:"),
                    intBox(0, 255, &DaemonPrefs::ipfilterLevel));
    ipfBox->addLayout(ipfForm);
    ipfBox->addWidget(boolBox(tr("Always filter LAN IPs"), &DaemonPrefs::filterLan));
    box->addWidget(ipf);
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildMessagesTab() {
    auto* w = new QWidget;
    auto* box = new QVBoxLayout(w);
    box->addWidget(boolBox(tr("Filter incoming messages (except current chat)"),
                           &DaemonPrefs::msgFilterEnabled));
    box->addWidget(boolBox(tr("Filter all messages"), &DaemonPrefs::msgFilterAll));
    box->addWidget(boolBox(tr("Filter messages from people not on your friend list"),
                           &DaemonPrefs::msgFilterFriends));
    box->addWidget(boolBox(tr("Filter messages from unknown clients"),
                           &DaemonPrefs::msgFilterSecure));
    box->addWidget(boolBox(tr("Filter messages containing keywords"),
                           &DaemonPrefs::msgFilterByKeyword));
    auto* form = new QFormLayout;
    form->addRow(tr("Keywords (',' separated):"),
                 strEdit(&DaemonPrefs::msgKeywords));
    box->addLayout(form);
    box->addStretch(1);
    return w;
}

QWidget* PreferencesPanel::buildRemoteTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(boolBox(tr("Run webserver on startup"),
                         &DaemonPrefs::webserverAutorun));
    form->addRow(tr("Web server TCP port:"),
                 intBox(0, 65535, &DaemonPrefs::webserverPort));
    form->addRow(boolBox(tr("Enable Low rights user (guest)"),
                         &DaemonPrefs::webserverGuest));
    form->addRow(boolBox(tr("Enable Gzip compression"),
                         &DaemonPrefs::webserverGzip));
    form->addRow(tr("Page refresh time (s):"),
                 intBox(0, 100'000, &DaemonPrefs::webserverRefresh));
    form->addRow(tr("Web template:"), strEdit(&DaemonPrefs::webserverTemplate));
    return w;
}

QWidget* PreferencesPanel::buildAdvancedTab() {
    auto* w = new QWidget;
    auto* form = new QFormLayout(w);
    form->addRow(tr("Max new connections / 5 s:"),
                 intBox(0, 1000, &DaemonPrefs::maxConnPer5Sec));
    form->addRow(tr("File buffer size (bytes):"),
                 intBox(0, 2'000'000'000, &DaemonPrefs::fileBufferSize));
    form->addRow(tr("Upload queue size (clients):"),
                 intBox(0, 1'000'000, &DaemonPrefs::uploadQueueSize));
    form->addRow(tr("Server connection refresh (s):"),
                 intBox(0, 1'000'000, &DaemonPrefs::serverKeepaliveTimeout));
    form->addRow(boolBox(tr("Verbose logging"), &DaemonPrefs::verbose));
    form->addRow(tr("Kad nodes URL:"), strEdit(&DaemonPrefs::kadNodesUrl));
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
