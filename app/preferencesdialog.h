// Dialog to view and edit the daemon's Connection + Directories preferences.
// Seeded from a DaemonPrefs snapshot; prefs() returns the edited values.

#pragma once

#include <QDialog>

#include "model/model.h"

class QCheckBox;
class QLineEdit;
class QSpinBox;

namespace amule {

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(const DaemonPrefs& prefs, QWidget* parent = nullptr);

    [[nodiscard]] DaemonPrefs prefs() const;

private:
    DaemonPrefs initial_;

    QSpinBox* maxDownload_ = nullptr;
    QSpinBox* maxUpload_ = nullptr;
    QSpinBox* slotAllocation_ = nullptr;
    QSpinBox* tcpPort_ = nullptr;
    QSpinBox* udpPort_ = nullptr;
    QSpinBox* maxSources_ = nullptr;
    QSpinBox* maxConnections_ = nullptr;
    QCheckBox* networkEd2k_ = nullptr;
    QCheckBox* networkKad_ = nullptr;
    QCheckBox* autoconnect_ = nullptr;
    QCheckBox* reconnect_ = nullptr;
    QLineEdit* incomingDir_ = nullptr;
    QLineEdit* tempDir_ = nullptr;
    QCheckBox* shareHidden_ = nullptr;
    QCheckBox* autoRescan_ = nullptr;
};

} // namespace amule
