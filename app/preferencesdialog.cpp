#include "preferencesdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace amule {
namespace {

QSpinBox* makeRate(quint64 value) {
    auto* box = new QSpinBox;
    box->setRange(0, 1'000'000);
    box->setSuffix(QStringLiteral(" kB/s"));
    box->setSpecialValueText(QStringLiteral("Unlimited")); // shown at 0
    box->setValue(static_cast<int>(value));
    return box;
}

QSpinBox* makeCount(quint64 value, int max) {
    auto* box = new QSpinBox;
    box->setRange(0, max);
    box->setValue(static_cast<int>(value));
    return box;
}

QSpinBox* makePort(quint64 value) {
    auto* box = new QSpinBox;
    box->setRange(0, 65535);
    box->setValue(static_cast<int>(value));
    return box;
}

} // namespace

PreferencesDialog::PreferencesDialog(const DaemonPrefs& prefs, QWidget* parent)
    : QDialog(parent), initial_(prefs) {
    setWindowTitle(QStringLiteral("Daemon preferences"));

    maxDownload_ = makeRate(prefs.maxDownload);
    maxUpload_ = makeRate(prefs.maxUpload);
    slotAllocation_ = makeRate(prefs.slotAllocation);
    tcpPort_ = makePort(prefs.tcpPort);
    udpPort_ = makePort(prefs.udpPort);
    maxSources_ = makeCount(prefs.maxSources, 5000);
    maxConnections_ = makeCount(prefs.maxConnections, 5000);
    networkEd2k_ = new QCheckBox(QStringLiteral("ed2k network"));
    networkEd2k_->setChecked(prefs.networkEd2k);
    networkKad_ = new QCheckBox(QStringLiteral("Kad network"));
    networkKad_->setChecked(prefs.networkKad);
    autoconnect_ = new QCheckBox(QStringLiteral("Auto-connect on start"));
    autoconnect_->setChecked(prefs.autoconnect);
    reconnect_ = new QCheckBox(QStringLiteral("Reconnect on loss"));
    reconnect_->setChecked(prefs.reconnect);
    incomingDir_ = new QLineEdit(prefs.incomingDir);
    tempDir_ = new QLineEdit(prefs.tempDir);
    shareHidden_ = new QCheckBox(QStringLiteral("Share hidden files"));
    shareHidden_->setChecked(prefs.shareHidden);
    autoRescan_ = new QCheckBox(QStringLiteral("Auto-rescan shared"));
    autoRescan_->setChecked(prefs.autoRescan);

    auto* connBox = new QGroupBox(QStringLiteral("Connection"));
    auto* connForm = new QFormLayout(connBox);
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

    auto* dirBox = new QGroupBox(QStringLiteral("Directories"));
    auto* dirForm = new QFormLayout(dirBox);
    dirForm->addRow(QStringLiteral("Incoming:"), incomingDir_);
    dirForm->addRow(QStringLiteral("Temp:"), tempDir_);
    dirForm->addRow(shareHidden_);
    dirForm->addRow(autoRescan_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(connBox);
    layout->addWidget(dirBox);
    layout->addWidget(buttons);
}

DaemonPrefs PreferencesDialog::prefs() const {
    DaemonPrefs p = initial_; // preserve loaded flag and any untouched fields
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
