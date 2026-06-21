#include "preferencespanel.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace amule {
namespace {

QSpinBox* makeRate() {
    auto* box = new QSpinBox;
    box->setRange(0, 1'000'000);
    box->setSuffix(QStringLiteral(" kB/s"));
    box->setSpecialValueText(QStringLiteral("Unlimited")); // shown at 0
    return box;
}

QSpinBox* makeCount(int max) {
    auto* box = new QSpinBox;
    box->setRange(0, max);
    return box;
}

QSpinBox* makePort() {
    auto* box = new QSpinBox;
    box->setRange(0, 65535);
    return box;
}

} // namespace

PreferencesPanel::PreferencesPanel(QWidget* parent) : QWidget(parent) {
    maxDownload_ = makeRate();
    maxUpload_ = makeRate();
    slotAllocation_ = makeRate();
    tcpPort_ = makePort();
    udpPort_ = makePort();
    maxSources_ = makeCount(5000);
    maxConnections_ = makeCount(5000);
    networkEd2k_ = new QCheckBox(QStringLiteral("ed2k network"));
    networkKad_ = new QCheckBox(QStringLiteral("Kad network"));
    autoconnect_ = new QCheckBox(QStringLiteral("Auto-connect on start"));
    reconnect_ = new QCheckBox(QStringLiteral("Reconnect on loss"));
    incomingDir_ = new QLineEdit;
    tempDir_ = new QLineEdit;
    shareHidden_ = new QCheckBox(QStringLiteral("Share hidden files"));
    autoRescan_ = new QCheckBox(QStringLiteral("Auto-rescan shared"));

    // Connection subtab.
    auto* connWidget = new QWidget;
    auto* connForm = new QFormLayout(connWidget);
    connForm->addRow(QStringLiteral("Max download:"), maxDownload_);
    connForm->addRow(QStringLiteral("Max upload:"), maxUpload_);
    connForm->addRow(QStringLiteral("Upload slot rate:"), slotAllocation_);
    connForm->addRow(QStringLiteral("TCP port:"), tcpPort_);
    connForm->addRow(QStringLiteral("UDP port:"), udpPort_);
    connForm->addRow(QStringLiteral("Max sources/file:"), maxSources_);
    connForm->addRow(QStringLiteral("Max connections:"), maxConnections_);
    connForm->addRow(networkEd2k_);
    connForm->addRow(networkKad_);
    connForm->addRow(autoconnect_);
    connForm->addRow(reconnect_);

    // Directories subtab.
    auto* dirWidget = new QWidget;
    auto* dirForm = new QFormLayout(dirWidget);
    dirForm->addRow(QStringLiteral("Incoming:"), incomingDir_);
    dirForm->addRow(QStringLiteral("Temp:"), tempDir_);
    dirForm->addRow(shareHidden_);
    dirForm->addRow(autoRescan_);

    auto* subtabs = new QTabWidget;
    subtabs->addTab(connWidget, QStringLiteral("Connection"));
    subtabs->addTab(dirWidget, QStringLiteral("Directories"));

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

void PreferencesPanel::setPrefs(DaemonPrefs prefs) {
    current_ = prefs;
    maxDownload_->setValue(static_cast<int>(prefs.maxDownload));
    maxUpload_->setValue(static_cast<int>(prefs.maxUpload));
    slotAllocation_->setValue(static_cast<int>(prefs.slotAllocation));
    tcpPort_->setValue(static_cast<int>(prefs.tcpPort));
    udpPort_->setValue(static_cast<int>(prefs.udpPort));
    maxSources_->setValue(static_cast<int>(prefs.maxSources));
    maxConnections_->setValue(static_cast<int>(prefs.maxConnections));
    networkEd2k_->setChecked(prefs.networkEd2k);
    networkKad_->setChecked(prefs.networkKad);
    autoconnect_->setChecked(prefs.autoconnect);
    reconnect_->setChecked(prefs.reconnect);
    incomingDir_->setText(prefs.incomingDir);
    tempDir_->setText(prefs.tempDir);
    shareHidden_->setChecked(prefs.shareHidden);
    autoRescan_->setChecked(prefs.autoRescan);
}

DaemonPrefs PreferencesPanel::collect() const {
    DaemonPrefs p = current_; // preserve loaded flag and any untouched fields
    p.maxDownload = static_cast<quint64>(maxDownload_->value());
    p.maxUpload = static_cast<quint64>(maxUpload_->value());
    p.slotAllocation = static_cast<quint64>(slotAllocation_->value());
    p.tcpPort = static_cast<quint64>(tcpPort_->value());
    p.udpPort = static_cast<quint64>(udpPort_->value());
    p.maxSources = static_cast<quint64>(maxSources_->value());
    p.maxConnections = static_cast<quint64>(maxConnections_->value());
    p.networkEd2k = networkEd2k_->isChecked();
    p.networkKad = networkKad_->isChecked();
    p.autoconnect = autoconnect_->isChecked();
    p.reconnect = reconnect_->isChecked();
    p.incomingDir = incomingDir_->text();
    p.tempDir = tempDir_->text();
    p.shareHidden = shareHidden_->isChecked();
    p.autoRescan = autoRescan_->isChecked();
    return p;
}

} // namespace amule
