// Preferences tab: the daemon's Connection + Directories preferences as
// subtabs, with Apply / Reload actions. Populated from the worker's prefsUpdated
// snapshot; emits applyRequested with the edited values.

#pragma once

#include <QWidget>

#include "model/model.h"

class QCheckBox;
class QLineEdit;
class QSpinBox;

namespace amule {

class PreferencesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PreferencesPanel(QWidget* parent = nullptr);

public slots:
    void setPrefs(amule::DaemonPrefs prefs);

signals:
    void applyRequested(amule::DaemonPrefs prefs);
    void reloadRequested();

private:
    [[nodiscard]] DaemonPrefs collect() const;

    DaemonPrefs current_; // base for collect(), preserves untouched fields

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
