// Small dialog to add an eD2k server manually (name + ip:port address).

#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

namespace amule {

class AddServerDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddServerDialog(QWidget* parent = nullptr);

    [[nodiscard]] QString serverName() const;
    [[nodiscard]] QString address() const;

private:
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* addressEdit_ = nullptr;
};

} // namespace amule
