// Preferences tab: every daemon preference amuled exposes over EC, grouped into
// subtabs (General, Connection, Files, Directories, Servers, Security, Messages,
// Remote, Online Sig, Advanced, Kademlia), with Apply / Reload. Widgets are
// bound to DaemonPrefs members via pointer-to-member to keep the wiring compact.

#pragma once

#include <QList>
#include <QWidget>

#include "model/model.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QWidget;

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
    struct BoolBinding {
        QCheckBox* widget;
        bool DaemonPrefs::* field;
    };
    struct IntBinding {
        QSpinBox* widget;
        quint64 DaemonPrefs::* field;
    };
    struct StringBinding {
        QLineEdit* widget;
        QString DaemonPrefs::* field;
    };

    [[nodiscard]] DaemonPrefs collect() const;

    // Widget factories that also register the binding.
    QCheckBox* boolBox(const QString& text, bool DaemonPrefs::* field);
    QSpinBox* intBox(int min, int max, quint64 DaemonPrefs::* field,
                     const QString& suffix = {}, bool unlimitedAtZero = false);
    QLineEdit* strEdit(QString DaemonPrefs::* field);

    QWidget* buildGeneralTab();
    QWidget* buildConnectionTab();
    QWidget* buildFilesTab();
    QWidget* buildDirectoriesTab();
    QWidget* buildServersTab();
    QWidget* buildSecurityTab();
    QWidget* buildMessagesTab();
    QWidget* buildRemoteTab();
    QWidget* buildAdvancedTab();

    DaemonPrefs current_;
    QList<BoolBinding> bools_;
    QList<IntBinding> ints_;
    QList<StringBinding> strings_;

    QComboBox* canSeeShares_ = nullptr;
};

} // namespace amule
